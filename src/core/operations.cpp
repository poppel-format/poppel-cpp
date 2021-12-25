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

#include "poppel/core/npy.hpp"
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


    // Validate node relative (normalized) path.
    //
    // Precondition:
    // - Path must be normalized before passed to this function. (not checked)
    //
    // Note:
    // - Path does not need to exist.
    // - Path must be relative.                                 (So no "/var" or "C:\\Program Files".)
    // - Path must not be empty, nor start with dot or dot-dot. (So no "", "." or "../treasure".)
    // - Path must have filename portion.                       (So no "trailing\\" or "trailing/".)
    bool is_valid_node_normalized_relpath(const std::filesystem::path& normalized_relpath) {
        if(normalized_relpath.is_absolute() || normalized_relpath.empty()) {
            return false;
        }
        auto pp0 = normalized_relpath.begin(); // Can be dereferenced because normalized_relpath is not empty.
        if(*pp0 == "." || *pp0 == "..") {
            return false;
        }
        return normalized_relpath.has_filename();
    }
    void assert_is_valid_node_normalized_relpath(const std::filesystem::path& normalized_relpath) {
        if(!is_valid_node_normalized_relpath(normalized_relpath)) {
            throw Exception("[" + normalized_relpath.string() + "] is not a valid relative path.");
        }
    }

    bool is_node_group(const Node& node) {
        return node.meta.type == NodeType::Group || node.meta.type == NodeType::File;
    }
    void assert_is_node_group(const Node& node) {
        if(!is_node_group(node)) {
            throw Exception("Node is not a group or file.");
        }
    }

    bool is_node_dataset(const Node& node) {
        return node.meta.type == NodeType::Dataset;
    }
    void assert_is_node_dataset(const Node& node) {
        if(!is_node_dataset(node)) {
            throw Exception("Node is not a dataset.");
        }
    }

    void assert_exists_directory(const std::filesystem::path& path) {
        if (!std::filesystem::is_directory(path)) {
            throw Exception("Path is not a directory.");
        }
    }
    void assert_not_exists(const std::filesystem::path& path) {
        if (std::filesystem::exists(path)) {
            throw Exception("Path is already occupied.");
        }
    }


    //----------------------------------
    // Node operations.
    //----------------------------------

    NodeMeta read_node_meta(const std::filesystem::path& nodepath) {
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
    void write_node_meta(const std::filesystem::path& nodepath, const NodeMeta& meta) {
        std::ofstream file(nodepath / "poppel.json");
        if (!file.is_open()) {
            throw Exception("Unable to open poppel.json file.");
        }
        nlohmann::json json;
        json["version"] = meta.version;
        json["type"] = text(meta.type);
        file << json;
    }

    Node get_file_node(const std::filesystem::path& name) {
        assert_exists_directory(name);

        auto meta = read_node_meta(name);
        if(meta.type != NodeType::File) {
            throw Exception("Node is not of file type.");
        }

        return Node { meta, std::move(name), };
    }
    Node create_file_node(const std::filesystem::path& name) {
        assert_not_exists(name);

        std::filesystem::create_directories(name);
        NodeMeta meta;
        meta.type = NodeType::File;
        write_node_meta(name, meta);

        return Node { meta, std::move(name), };
    }
    Node require_file_node(const std::filesystem::path& name) {
        if(std::filesystem::is_directory(name)) {
            return get_file_node(name);
        }
        else {
            return create_file_node(name);
        }
    }
    void delete_file_node(const std::filesystem::path& name) {
        assert_exists_directory(name);
        std::filesystem::remove_all(name);
    }

    bool has_node(const Node& node, const std::filesystem::path& name, const FileStates& filestates, NodeType nodetype) {
        assert_file_open(filestates);
        assert_is_node_group(node);
        auto normalized_name = name.lexically_normal();
        assert_is_valid_node_normalized_relpath(normalized_name);

        auto dirpath = node.path() / normalized_name;
        if(!std::filesystem::is_directory(dirpath)) {
            return false;
        }

        auto meta = read_node_meta(dirpath);
        if(meta.type != nodetype) {
            return false;
        }
        return true;
    }
    Node get_node(const Node& node, const std::filesystem::path& name, const FileStates& filestates, NodeType nodetype) {
        assert_file_open(filestates);
        assert_is_node_group(node);
        auto normalized_name = name.lexically_normal();
        assert_is_valid_node_normalized_relpath(normalized_name);

        auto dirpath = node.path() / normalized_name;
        assert_exists_directory(dirpath);

        auto meta = read_node_meta(dirpath);
        if(meta.type != nodetype) {
            throw Exception("Node is not of expected type.");
        }

        return Node { meta, node.root, node.relpath / normalized_name, };
    }
    // Will not create nodes for intermediate directories.
    Node create_node_immediate(const Node& node, const std::filesystem::path& name, const FileStates& filestates, NodeType nodetype) {
        assert_file_writable(filestates);
        assert_is_node_group(node);
        auto normalized_name = name.lexically_normal();
        assert_is_valid_node_normalized_relpath(normalized_name);

        auto dirpath = node.path() / normalized_name;
        assert_not_exists(dirpath);

        std::filesystem::create_directory(dirpath);
        NodeMeta meta;
        meta.type = nodetype;
        write_node_meta(dirpath, meta);

        return Node { meta, node.root, normalized_name, };
    }
    // Note:
    // - All intermediate directories must be groups or non-exist.
    Node require_node(const Node& node, const std::filesystem::path& name, const FileStates& filestates, NodeType nodetype) {
        assert_file_open(filestates);
        assert_is_node_group(node);
        auto normalized_name = name.lexically_normal();
        assert_is_valid_node_normalized_relpath(normalized_name);

        auto cur_node = node;
        for(auto it = normalized_name.begin(); it != normalized_name.end(); ++it) {
            const NodeType required_node_type = std::next(it) == normalized_name.end() ? nodetype : NodeType::Group;
            const auto& part = *it;

            if(std::filesystem::is_directory(cur_node.path() / part)) {
                cur_node = get_node(cur_node, part, filestates, required_node_type);
            } else {
                cur_node = create_node_immediate(cur_node, part, filestates, required_node_type);
            }
        }
        return cur_node;
    }
    Node create_node(const Node& node, const std::filesystem::path& name, const FileStates& filestates, NodeType nodetype) {
        assert_file_writable(filestates);
        assert_is_node_group(node);
        auto normalized_name = name.lexically_normal();
        assert_is_valid_node_normalized_relpath(normalized_name);

        auto cur_node = node;
        if(normalized_name.has_parent_path()) {
            // Has parent path.
            cur_node = require_node(cur_node, normalized_name.parent_path(), filestates, NodeType::Group);
        }
        return create_node_immediate(cur_node, normalized_name.filename(), filestates, nodetype);
    }
    void delete_node(const Node& node, const std::filesystem::path& name, const FileStates& filestates) {
        assert_file_writable(filestates);
        assert_is_node_group(node);
        auto normalized_name = name.lexically_normal();
        assert_is_valid_node_normalized_relpath(normalized_name);

        auto dirpath = node.path() / normalized_name;
        assert_exists_directory(dirpath);
        std::filesystem::remove_all(dirpath);
    }

    Attribute get_attribute(const Node& node, const FileStates& filestates) {
        assert_file_open(filestates);
        auto jsonfilepath = node.root / node.relpath / "attributes.json";

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

    DatasetMeta load_npy_meta(const std::filesystem::path& npyfile) {
        npy::Header header = npy::load_header(npyfile);
        DatasetMeta meta;
        meta.shape         = header.shape;
        meta.wordsize      = header.dtype.itemsize;
        meta.fortran_order = header.fortran_order;
        return meta;
    }


    void load_to(std::string& val, const std::filesystem::path& path) { npy::load(path, val); }

    void save_from(const std::string& val, const std::filesystem::path& path) { npy::save(path, val); }

    //----------------------------------
    // DataSet operations.
    //----------------------------------

    Json load_attr(const Attribute& attr) {
        std::ifstream ifs(attr.jsonfile);
        if (!ifs.is_open()) {
            throw std::runtime_error("Failed to open attribute file: " + attr.jsonfile.string());
        }

        Json j;
        ifs >> j;
        return j;
    }
    void save_attr(const Json& val, const Attribute& attr) {
        std::ofstream ofs(attr.jsonfile);
        if (!ofs.is_open()) {
            throw std::runtime_error("Failed to open attribute file: " + attr.jsonfile.string());
        }

        ofs << val;
    }

} // namespace poppel::core
