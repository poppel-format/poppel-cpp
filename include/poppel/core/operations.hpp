#ifndef INCLUDE_POPPEL_CORE_OPERATIONS_HPP_
#define INCLUDE_POPPEL_CORE_OPERATIONS_HPP_

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>

#include "structs.hpp"

namespace poppel::core {

    //----------------------------------
    // Utility functions
    //----------------------------------

    inline void assert_file_open(const FileStates& filestates) {
        if (filestates.open_state == FileOpenState::Closed) {
            throw std::runtime_error("Unable to operate on closed File instance.");
        }
    }

    inline void assert_file_writable(const FileStates& filestates) {
        if (filestates.open_state == FileOpenState::Closed) {
            throw std::runtime_error("Unable to operate on closed File instance.");
        }
        if (filestates.open_state == FileOpenState::ReadOnly) {
            throw std::runtime_error("Cannot change data on file in read only mode");
        }
    }

    //----------------------------------
    // Node operations.
    //----------------------------------

    // Warning: Does not test if inside poppel directory. Only test if the object can be an poppel object (i.e. a directory).
    inline bool is_poppel_object(const std::filesystem::path& path) {
        return std::filesystem::is_directory(path);
    }

    inline bool contains(const Node& node, std::string_view name, const FileStates& filestates) {
        if(filestates.open_state == FileOpenState::Closed) {
            return false;
        }
        if(name == ".") {
            return true;
        }
        if(name == "") {
            return false;
        }
        const auto directory = node.path / name;
        return is_poppel_object(directory);
    }

    inline Node getitem(const Node& node, std::string_view name, const FileStates& filestates) {
        assert_file_open(filestates);
        auto directorypath = node.path / name;

        if(!contains(node, name, filestates)) {
            std::cout << "Error: " << name << " is not a valid poppel object." << std::endl;
            throw std::runtime_error("Node does not exist.");
        }

        return Node { std::move(directorypath) };
    }

} // namespace poppel::core

#endif
