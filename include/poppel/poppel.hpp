#ifndef INCLUDE_POPPEL_POPPEL_HPP_
#define INCLUDE_POPPEL_POPPEL_HPP_

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string_view>

#include "core/operations.hpp"

namespace poppel {

    class Group {
    private:
        core::Node node_;

    public:
        Group(std::string_view relpath) {
            std::cout << "Group: " << relpath << std::endl;
        }
    };

    class File {
    private:
        core::Node       node_;
        core::FileStates states_;

    public:
        using ModeType = std::int8_t;
        static constexpr ModeType ReadOnly = 1;
        // TODO etc.

        File(const std::filesystem::path& path, ModeType mode) {
            std::cout << "File: " << path << " mode: " << +mode << std::endl;
        }
    };

    class DataSet {
    private:
        core::Node node_;

    public:
        DataSet() = default;
    };

} // namespace poppel

#endif
