#ifndef INCLUDE_POPPEL_CORE_OPERATIONS_HPP_
#define INCLUDE_POPPEL_CORE_OPERATIONS_HPP_

#include "types.hpp"
#include "utilities.hpp"

namespace poppel::core {

    //----------------------------------
    // Utility functions
    //----------------------------------

    void assert_file_open(const FileStates& filestates);
    void assert_file_writable(const FileStates& filestates);

    bool is_valid_node_normalized_relpath(const std::filesystem::path& normalized_relpath);
    void assert_is_valid_node_normalized_relpath(const std::filesystem::path& normalized_relpath);

    bool is_node_group(const Node& node);
    void assert_is_node_group(const Node& node);

    bool is_node_dataset(const Node& node);
    void assert_is_node_dataset(const Node& node);

    void assert_exists_directory(const std::filesystem::path& path);
    void assert_not_exists(const std::filesystem::path& path);

    //----------------------------------
    // Node operations.
    //----------------------------------

    NodeMeta read_node_meta(const std::filesystem::path& nodepath, const FileStates& filestates);
    void write_node_meta(const std::filesystem::path& nodepath, const NodeMeta& meta, const FileStates& filestates);

    Node get_file_node(const std::filesystem::path& name);
    Node create_file_node(const std::filesystem::path& name);
    Node require_file_node(const std::filesystem::path& name);
    void delete_file_node(const std::filesystem::path& name);

    // If directory does not exist or node type does not match, an error is thrown.
    Node get_node    (const Node& node, const std::filesystem::path& name, const FileStates& filestates, NodeType nodetype);
    Node create_node (const Node& node, const std::filesystem::path& name, const FileStates& filestates, NodeType nodetype);
    // If the required node does not exist, a new node is created.
    Node require_node(const Node& node, const std::filesystem::path& name, const FileStates& filestates, NodeType nodetype);
    void delete_node (const Node& node, const std::filesystem::path& name, const FileStates& filestates);

    Attribute get_attribute(const Node& node, const FileStates& filestates);

    //----------------------------------
    // DataSet operations.
    //----------------------------------

    DatasetMeta load_npy_meta(const std::filesystem::path& dataset);

    void load_to(bool& val,                  const std::filesystem::path& dataset);
    void load_to(std::int8_t& val,           const std::filesystem::path& dataset);
    void load_to(std::int16_t& val,          const std::filesystem::path& dataset);
    void load_to(std::int32_t& val,          const std::filesystem::path& dataset);
    void load_to(std::int64_t& val,          const std::filesystem::path& dataset);
    void load_to(std::uint8_t& val,          const std::filesystem::path& dataset);
    void load_to(std::uint16_t& val,         const std::filesystem::path& dataset);
    void load_to(std::uint32_t& val,         const std::filesystem::path& dataset);
    void load_to(std::uint64_t& val,         const std::filesystem::path& dataset);
    void load_to(float& val,                 const std::filesystem::path& dataset);
    void load_to(double& val,                const std::filesystem::path& dataset);
    void load_to(std::complex<float>& val,   const std::filesystem::path& dataset);
    void load_to(std::complex<double>& val,  const std::filesystem::path& dataset);

    template< typename T, std::enable_if_t< std::is_integral_v<T>>* = nullptr >
    void load_to(T& val, const std::filesystem::path& dataset) {
        load_to(static_cast<NormalizedIntegralOf<T>&>(val), dataset);
    }

    void load_to(std::vector<std::int8_t>& val,           const std::filesystem::path& dataset);
    void load_to(std::vector<std::int16_t>& val,          const std::filesystem::path& dataset);
    void load_to(std::vector<std::int32_t>& val,          const std::filesystem::path& dataset);
    void load_to(std::vector<std::int64_t>& val,          const std::filesystem::path& dataset);
    void load_to(std::vector<std::uint8_t>& val,          const std::filesystem::path& dataset);
    void load_to(std::vector<std::uint16_t>& val,         const std::filesystem::path& dataset);
    void load_to(std::vector<std::uint32_t>& val,         const std::filesystem::path& dataset);
    void load_to(std::vector<std::uint64_t>& val,         const std::filesystem::path& dataset);
    void load_to(std::vector<float>& val,                 const std::filesystem::path& dataset);
    void load_to(std::vector<double>& val,                const std::filesystem::path& dataset);
    void load_to(std::vector<std::complex<float>>& val,   const std::filesystem::path& dataset);
    void load_to(std::vector<std::complex<double>>& val,  const std::filesystem::path& dataset);

    void load_to(std::string& val, const std::filesystem::path& dataset);


    void save_from(bool val,                  const std::filesystem::path& dataset);
    void save_from(std::int8_t val,           const std::filesystem::path& dataset);
    void save_from(std::int16_t val,          const std::filesystem::path& dataset);
    void save_from(std::int32_t val,          const std::filesystem::path& dataset);
    void save_from(std::int64_t val,          const std::filesystem::path& dataset);
    void save_from(std::uint8_t val,          const std::filesystem::path& dataset);
    void save_from(std::uint16_t val,         const std::filesystem::path& dataset);
    void save_from(std::uint32_t val,         const std::filesystem::path& dataset);
    void save_from(std::uint64_t val,         const std::filesystem::path& dataset);
    void save_from(float val,                 const std::filesystem::path& dataset);
    void save_from(double val,                const std::filesystem::path& dataset);
    void save_from(std::complex<float> val,   const std::filesystem::path& dataset);
    void save_from(std::complex<double> val,  const std::filesystem::path& dataset);

    template< typename T, std::enable_if_t< std::is_integral_v<T>>* = nullptr >
    void save_from(T val, const std::filesystem::path& dataset) {
        save_from(static_cast<NormalizedIntegralOf<T>>(val), dataset);
    }

    void save_from(const std::vector<std::int8_t>& val,           const std::filesystem::path& dataset);
    void save_from(const std::vector<std::int16_t>& val,          const std::filesystem::path& dataset);
    void save_from(const std::vector<std::int32_t>& val,          const std::filesystem::path& dataset);
    void save_from(const std::vector<std::int64_t>& val,          const std::filesystem::path& dataset);
    void save_from(const std::vector<std::uint8_t>& val,          const std::filesystem::path& dataset);
    void save_from(const std::vector<std::uint16_t>& val,         const std::filesystem::path& dataset);
    void save_from(const std::vector<std::uint32_t>& val,         const std::filesystem::path& dataset);
    void save_from(const std::vector<std::uint64_t>& val,         const std::filesystem::path& dataset);
    void save_from(const std::vector<float>& val,                 const std::filesystem::path& dataset);
    void save_from(const std::vector<double>& val,                const std::filesystem::path& dataset);
    void save_from(const std::vector<std::complex<float>>& val,   const std::filesystem::path& dataset);
    void save_from(const std::vector<std::complex<double>>& val,  const std::filesystem::path& dataset);

    void save_from(const std::string& val, const std::filesystem::path& dataset);

    //----------------------------------
    // Attribute operations.
    //----------------------------------

    Json load_attr(const Attribute& attr);
    void save_attr(const Json& val, const Attribute& attr); 

} // namespace poppel::core

#endif
