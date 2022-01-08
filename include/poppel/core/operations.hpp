#ifndef INCLUDE_POPPEL_CORE_OPERATIONS_HPP_
#define INCLUDE_POPPEL_CORE_OPERATIONS_HPP_

#include "npy.hpp"
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

    bool is_node_raw(const Node& node);
    void assert_is_node_raw(const Node& node);

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

    bool has_node    (const Node& node, const std::filesystem::path& name, const FileStates& filestates, NodeType nodetype);
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

    // Loading data.
    //----------------------------------

    // Generic loading for any scalar data.
    template< typename T, std::enable_if_t< npy::is_scalar<T>>* = nullptr >
    void load_to(T& val, const std::filesystem::path& path) {
        npy::load(path, val);
    }

    // Generic loading for any 1D scalar data.
    template< typename T, std::enable_if_t< npy::is_scalar<T>>* = nullptr >
    void load_to(T* data, Size size, const std::filesystem::path& path) {
        npy::load(path, npy::create_header<T>(false, { size }), reinterpret_cast<std::byte*>(data));
    }

    // Generic loading for any dimension scalar data.
    template< typename T, std::enable_if_t< npy::is_scalar<T>>* = nullptr >
    void load_to(T* data, bool fortran_order, std::vector<Size> shape, const std::filesystem::path& path) {
        npy::load(path, npy::create_header<T>(fortran_order, std::move(shape)), reinterpret_cast<std::byte*>(data));
    }

    // Load std::vector.
    template< typename T, std::enable_if_t< npy::is_scalar<T> && !std::is_same_v<T, bool> >* = nullptr >
    void load_to(std::vector<T>& val, const std::filesystem::path& path) {
        npy::load(path, val);
    }
    // Load std::string.
    void load_to(std::string& val, const std::filesystem::path& path);


    // Saving data.
    //----------------------------------

    // Generic saving for any scalar data.
    template< typename T, std::enable_if_t< npy::is_scalar<T>>* = nullptr >
    void save_from(T val, const std::filesystem::path& path) {
        npy::save(path, val);
    }

    // Generic saving for any 1D scalar data.
    template< typename T, std::enable_if_t< npy::is_scalar<T>>* = nullptr >
    void save_from(const T* data, Size size, const std::filesystem::path& path) {
        npy::save(path, npy::create_header<T>(false, { size }), reinterpret_cast<const std::byte*>(data));
    }

    // Generic saving for any dimension scalar data.
    template< typename T, std::enable_if_t< npy::is_scalar<T>>* = nullptr >
    void save_from(const T* data, bool fortran_order, std::vector<Size> shape, const std::filesystem::path& path) {
        npy::save(path, npy::create_header<T>(fortran_order, std::move(shape)), reinterpret_cast<const std::byte*>(data));
    }

    // Save std::vector.
    template< typename T, std::enable_if_t< npy::is_scalar<T> && !std::is_same_v<T, bool> >* = nullptr >
    void save_from(const std::vector<T>& val, const std::filesystem::path& path) {
        npy::save(path, val);
    }
    // Save string.
    void save_from(const std::string& val, const std::filesystem::path& path);

    //----------------------------------
    // Attribute operations.
    //----------------------------------

    Json load_attr(const Attribute& attr);
    void save_attr(const Json& val, const Attribute& attr); 

} // namespace poppel::core

#endif
