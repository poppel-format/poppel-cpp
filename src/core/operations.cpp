#include <array>
#include <cassert>
#include <complex>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <iostream>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>

#include "core/npy.hpp"
#include "poppel/core/exceptions.hpp"
#include "poppel/core/operations.hpp"

namespace poppel::core {

    //----------------------------------
    // Utility functions
    //----------------------------------

    void assert_file_open(const FileStates& filestates) {
        if (filestates.open_state == FileOpenState::Closed) {
            throw Exception("Unable to operate on closed File instance.");
        }
    }

    void assert_file_writable(const FileStates& filestates) {
        if (filestates.open_state == FileOpenState::Closed) {
            throw Exception("Unable to operate on closed File instance.");
        }
        if (filestates.open_state == FileOpenState::ReadOnly) {
            throw Exception("Cannot change data on file in read only mode");
        }
    }

    //----------------------------------
    // Node operations.
    //----------------------------------

    // Warning: Does not test if inside poppel directory. Only test if the object can be an poppel object (i.e. a directory).
    bool is_poppel_dir(const std::filesystem::path& path) {
        return std::filesystem::is_directory(path);
    }
    void assert_is_poppel_dir(const std::filesystem::path& path) {
        if (!is_poppel_dir(path)) {
            throw Exception("Path is not a directory.");
        }
    }

    bool contains_dir(const Node& node, const std::filesystem::path& name, const FileStates& filestates) {
        assert_file_open(filestates);
        if(name == ".") {
            return true;
        }
        if(name == "") {
            return false;
        }
        const auto directory = node.path / name;
        return is_poppel_dir(directory);
    }
    void assert_contains_dir(const Node& node, const std::filesystem::path& name, const FileStates& filestates) {
        if(!contains_dir(node, name, filestates)) {
            throw Exception("Directory does not exist.");
        }
    }
    void assert_not_contains_dir(const Node& node, const std::filesystem::path& name, const FileStates& filestates) {
        if(contains_dir(node, name, filestates)) {
            throw Exception("Directory already exists.");
        }
    }

    NodeMeta read_node_meta(const std::filesystem::path& nodepath, const FileStates& filestates) {
        assert_file_open(filestates);
        std::ifstream file(nodepath / "poppel.json");
        if (!file.is_open()) {
            throw Exception("Unable to open poppel.json file.");
        }
        nlohmann::json json;
        file >> json;
        return {
            json["version"],
            node_type(json["type"]),
        };
    }
    void write_node_meta(const std::filesystem::path& nodepath, const NodeMeta& meta, const FileStates& filestates) {
        assert_file_writable(filestates);
        std::ofstream file(nodepath / "poppel.json");
        if (!file.is_open()) {
            throw Exception("Unable to open poppel.json file.");
        }
        nlohmann::json json;
        json["version"] = meta.version;
        json["type"] = text(meta.type);
        file << json;
    }

    Node get_node(const std::filesystem::path& name, const FileStates& filestates, NodeType nodetype) {
        assert_file_open(filestates);
        assert_is_poppel_dir(name);

        auto meta = read_node_meta(name, filestates);
        if(meta.type != nodetype) {
            throw Exception("Node is not of the expected type.");
        }

        return Node { meta, std::move(name) };
    }
    Node get_node(const Node& node, const std::filesystem::path& name, const FileStates& filestates, NodeType nodetype) {
        return get_node(node.path / name, filestates, nodetype);
    }
    // Will not create nodes for intermediate directories.
    Node create_node_immediate(const Node& node, const std::filesystem::path& name, const FileStates& filestates, NodeType nodetype) {
        assert_file_writable(filestates);
        assert_not_contains_dir(node, name, filestates);
        auto directorypath = node.path / name;

        std::filesystem::create_directory(directorypath);
        NodeMeta meta;
        meta.type = nodetype;

        return Node { meta, std::move(directorypath) };
    }
    // Note:
    // - All intermediate directories must be groups or non-exist.
    Node require_node(const Node& node, const std::filesystem::path& name, const FileStates& filestates, NodeType nodetype) {
        assert_file_open(filestates);
        const auto& relpath = name;

        auto cur_node = node;
        for(auto it = relpath.begin(); it != relpath.end(); ++it) {
            const NodeType required_node_type = std::next(it) == relpath.end() ? nodetype : NodeType::Group;
            const auto& part = *it;

            if(contains_dir(cur_node, part, filestates)) {
                cur_node = get_node(cur_node, part, filestates, required_node_type);
            } else {
                cur_node = create_node_immediate(cur_node, part, filestates, required_node_type);
            }
        }
        return cur_node;
    }
    Node create_node(const Node& node, const std::filesystem::path& name, const FileStates& filestates, NodeType nodetype) {
        assert_file_writable(filestates);
        const auto relpath = std::filesystem::path(name);
        const auto relpathparent = relpath.parent_path();

        if(relpathparent != relpath) {
            // Has parent path.
            require_node(node, relpathparent, filestates, NodeType::Group);
        }
        create_node_immediate(node, relpath, filestates, nodetype);
    }
    void delete_node(const Node& node, const std::filesystem::path& name, const FileStates& filestates) {
        assert_file_writable(filestates);
        assert_contains_dir(node, name, filestates);
        const auto directorypath = node.path / name;
        std::filesystem::remove_all(directorypath);
    }

    Attribute get_attribute(const Node& node, const FileStates& filestates) {
        assert_file_open(filestates);
        auto jsonfilepath = node.path / "attributes.json";

        if(!std::filesystem::exists(jsonfilepath)) {
            if(filestates.open_state == FileOpenState::ReadOnly) {
                throw std::runtime_error("Cannot change data on file in read only mode");
            }
            else if(filestates.open_state == FileOpenState::ReadWrite) {
                std::ofstream ofs { jsonfilepath };
                ofs << "{}";
            }
        }
        return Attribute { std::move(jsonfilepath) };
    }

    //----------------------------------
    // DataSet operations.
    //----------------------------------

    DataSetMeta load_npy_meta(const std::filesystem::path& dataset) {
        npy::Header header = npy::load_header(dataset);
        DataSetMeta meta;
        meta.shape         = header.shape;
        meta.wordsize      = header.dtype.itemsize;
        meta.fortran_order = header.fortran_order;
        return meta;
    }

    void load_to(bool& val,                  const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::int8_t& val,           const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::int16_t& val,          const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::int32_t& val,          const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::int64_t& val,          const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::uint8_t& val,          const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::uint16_t& val,         const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::uint32_t& val,         const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::uint64_t& val,         const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(float& val,                 const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(double& val,                const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::complex<float>& val,   const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::complex<double>& val,  const std::filesystem::path& dataset) { npy::load(dataset, val); }

    void load_to(std::vector<std::int8_t>& val,           const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::vector<std::int16_t>& val,          const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::vector<std::int32_t>& val,          const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::vector<std::int64_t>& val,          const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::vector<std::uint8_t>& val,          const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::vector<std::uint16_t>& val,         const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::vector<std::uint32_t>& val,         const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::vector<std::uint64_t>& val,         const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::vector<float>& val,                 const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::vector<double>& val,                const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::vector<std::complex<float>>& val,   const std::filesystem::path& dataset) { npy::load(dataset, val); }
    void load_to(std::vector<std::complex<double>>& val,  const std::filesystem::path& dataset) { npy::load(dataset, val); }

    void load_to(std::string& val, const std::filesystem::path& dataset) { npy::load(dataset, val); }


    void save_from(bool val,                  const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(std::int8_t val,           const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(std::int16_t val,          const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(std::int32_t val,          const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(std::int64_t val,          const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(std::uint8_t val,          const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(std::uint16_t val,         const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(std::uint32_t val,         const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(std::uint64_t val,         const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(float val,                 const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(double val,                const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(std::complex<float> val,   const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(std::complex<double> val,  const std::filesystem::path& dataset) { npy::save(dataset, val); }

    void save_from(const std::vector<std::int8_t>& val,           const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(const std::vector<std::int16_t>& val,          const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(const std::vector<std::int32_t>& val,          const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(const std::vector<std::int64_t>& val,          const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(const std::vector<std::uint8_t>& val,          const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(const std::vector<std::uint16_t>& val,         const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(const std::vector<std::uint32_t>& val,         const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(const std::vector<std::uint64_t>& val,         const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(const std::vector<float>& val,                 const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(const std::vector<double>& val,                const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(const std::vector<std::complex<float>>& val,   const std::filesystem::path& dataset) { npy::save(dataset, val); }
    void save_from(const std::vector<std::complex<double>>& val,  const std::filesystem::path& dataset) { npy::save(dataset, val); }

    void save_from(const std::string& val, const std::filesystem::path& dataset) { npy::save(dataset, val); }


} // namespace poppel::core
