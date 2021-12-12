#ifndef INCLUDE_POPPEL_CORE_STRUCTS_HPP_
#define INCLUDE_POPPEL_CORE_STRUCTS_HPP_

// This file defines core data structures for Poppel.
// The structures are not meant to be used directly by the user.
// Circular dependencies are not allowed.
// Functions involving multiple components are not recommended.

#include <filesystem>

namespace poppel::core {

    // Represents a node in tree traversal.
    // In poppel, it could represent a file/group/dataset/raw.
    // In file system, it is a directory containing the required metadata.
    struct Node {
        std::filesystem::path path;
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

} // namespace poppel::core

#endif
