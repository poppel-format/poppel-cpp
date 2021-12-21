#ifndef INCLUDE_POPPEL_POPPEL_HPP_
#define INCLUDE_POPPEL_POPPEL_HPP_

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#include "core/exceptions.hpp"
#include "core/operations.hpp"

namespace poppel {

    class Group {
    private:
        core::Node node_;
        core::FileStates* pstates_ = nullptr;

    public:
        Group(core::Node node, core::FileStates* pstates) :
            node_(std::move(node)), pstates_(pstates)
        {}

        auto attr() const {
            return core::get_attribute(node_, *pstates_);
        }

        auto get_group(const std::filesystem::path& name) const {
            return Group(
                core::get_node(node_, name, *pstates_, core::NodeType::Group),
                pstates_
            );
        }
        auto create_group(const std::filesystem::path& name) const {
            return Group (
                core::create_node(node_, name, *pstates_, core::NodeType::Group),
                pstates_
            );
        }
        auto require_group(const std::filesystem::path& name) const {
            return Group (
                core::require_node(node_, name, *pstates_, core::NodeType::Group),
                pstates_
            );
        }
        void delete_group(const std::filesystem::path& name) const {
            core::delete_node(node_, name, *pstates_);
        }
    };

    class File {
    private:
        core::Node                        node_;
        // Using heap allocation to keep states_ in a fixed address.
        std::unique_ptr<core::FileStates> pstates_;

    public:
        using ModeType = std::int8_t;
        static constexpr ModeType Read = 1;
        static constexpr ModeType Write = 2;

        static constexpr ModeType ReadWrite = Read | Write;

        File(const std::filesystem::path& path, ModeType mode)
        {
            if(mode & Write) {
                if(mode & Read) {
                    pstates_ = std::make_unique<core::FileStates>();
                    pstates_->open_state = core::FileOpenState::ReadWrite;
                }
                else {
                    throw Exception("File: write mode requires read mode");
                }
            }
            else if(mode & Read) {
                pstates_ = std::make_unique<core::FileStates>();
                pstates_->open_state = core::FileOpenState::ReadOnly;
            }
            else {
                throw Exception("File: invalid mode");
            }

            node_ = core::get_node(path, *pstates_, core::NodeType::File);
        }

        ~File() {
            close();
        }

        File(File&&) = default;

        void close() {
            pstates_->open_state = core::FileOpenState::Closed;
        }
    };

    // The view of actual dataset.
    class DataSet {
    private:
        core::Node          node_;
        core::FileStates*   pstates_ = nullptr;

    public:
        DataSet(core::Node node, core::FileStates* pstates) :
            node_(std::move(node)), pstates_(pstates)
        {}

        template< typename T >
        void load_to(T& val) const {
            core::assert_file_open(*pstates_);
            core::load_to(val, node_.path / "data.npy");
        }

        template< typename T >
        void save_from(const T& val) const {
            core::assert_file_writable(*pstates_);
            core::save_from(val, node_.path / "data.npy");
        }
    };

} // namespace poppel

#endif
