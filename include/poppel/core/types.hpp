#ifndef INCLUDE_POPPEL_CORE_TYPES_HPP_
#define INCLUDE_POPPEL_CORE_TYPES_HPP_

// This file defines core data structures for Poppel.
// The structures are not meant to be used directly by the user.
// Circular dependencies are not allowed.
// Functions involving multiple components are not recommended.

#include <complex>
#include <cstddef>
#include <cstdint>
#include <filesystem>

#include <nlohmann/json.hpp>

namespace poppel {

    using Index = std::ptrdiff_t;
    using Size  = std::ptrdiff_t;

    using Json = nlohmann::json;

    namespace core {
        // Meta data.
        //------------------------------

        enum class NodeType {
            Unknown,
            File,
            Group,
            Dataset,
            Raw,
        };
        constexpr const char* text(NodeType val) {
            switch (val) {
                case NodeType::File:    return "file";
                case NodeType::Group:   return "group";
                case NodeType::Dataset: return "dataset";
                case NodeType::Raw:     return "raw";
                default:                return "";
            }
        }
        constexpr NodeType node_type(std::string_view name) {
            if (name == "file") {
                return NodeType::File;
            } else if (name == "group") {
                return NodeType::Group;
            } else if (name == "dataset") {
                return NodeType::Dataset;
            } else if (name == "raw") {
                return NodeType::Raw;
            } else {
                return NodeType::Unknown;
            }
        }

        struct NodeMeta {
            int      version = 1;
            NodeType type = NodeType::Unknown;
        };

        // Represents a node in tree traversal.
        // In poppel, it could represent a file/group/dataset/raw.
        // In file system, it is a directory containing the required metadata.
        struct Node {
            NodeMeta meta;
            std::filesystem::path root;
            std::filesystem::path relpath;

            auto path() const {
                return relpath.empty() ? root : root / relpath;
            }
        };

        enum class FileOpenState {
            ReadOnly,
            ReadWrite,
            Closed,
        };

        // File states that are not a part of the general node state.
        struct FileStates {
            FileOpenState open_state = FileOpenState::Closed;
        };


        struct DatasetMeta {
            std::vector<Size> shape;
            Size              wordsize = 1;
            // Whether the first dimension is the fastest changing one, also known as column major for matrix.
            bool              fortran_order = false;
        };

        struct Attribute {
            std::filesystem::path jsonfile;
        };

    } // namespace core

} // namespace poppel

#endif
