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

    // Forward declarations.
    class Group;
    class File;
    class Dataset;

    class Group {
    private:
        core::Node          node_;
        core::FileStates*   pstates_ = nullptr;

    public:
        Group() = default;
        Group(core::Node node, core::FileStates* pstates):
            node_(std::move(node)), pstates_(pstates)
        {}

        Group& operator=(const Group&) = default;
        Group& operator=(Group&&     ) = default;

        // Group management.
        Group get_group(const std::filesystem::path& name) const;
        Group create_group(const std::filesystem::path& name) const;
        Group require_group(const std::filesystem::path& name) const;
        void delete_group(const std::filesystem::path& name) const;

        // Dataset management.
        Dataset get_dataset(const std::filesystem::path& name) const;
        Dataset create_dataset(const std::filesystem::path& name) const;
        Dataset require_dataset(const std::filesystem::path& name) const;
        void delete_dataset(const std::filesystem::path& name) const;

        // Attributes.
        auto load_attr() const {
            return core::load_attr(core::get_attribute(node_, *pstates_));
        }
        auto save_attr(const Json& val) const {
            return core::save_attr(val, core::get_attribute(node_, *pstates_));
        }
    };

    class Dataset {
    private:
        core::Node          node_;
        core::FileStates*   pstates_ = nullptr;

    public:
        Dataset(core::Node node, core::FileStates* pstates):
            node_(std::move(node)), pstates_(pstates)
        {}

        // Dataset operations.
        template< typename T >
        void load_to(T& val) const {
            core::assert_file_open(*pstates_);
            core::assert_is_node_dataset(node_);
            core::load_to(val, node_.path() / "data.npy");
        }

        template< typename T >
        void save_from(const T& val) const {
            core::assert_file_writable(*pstates_);
            core::assert_is_node_dataset(node_);
            core::save_from(val, node_.path() / "data.npy");
        }

        // Attributes.
        auto load_attr() const {
            return core::load_attr(core::get_attribute(node_, *pstates_));
        }
        auto save_attr(const Json& val) const {
            return core::save_attr(val, core::get_attribute(node_, *pstates_));
        }
    };


    class File {
    public:
        using ModeType = std::int8_t;
        // File visibility/modification mode. Cannot be write-only.
        static constexpr ModeType Read = 1;
        static constexpr ModeType Write = 2;
        // If file does not exist: Fails if none is set. Creates a new file if Create is set.
        static constexpr ModeType Create = 4;
        // If file already exists: Preserves if none is set. Fails if Excl is set. Truncate the file is Truncate is set. Cannot be both set.
        static constexpr ModeType Excl = 8;
        static constexpr ModeType Truncate = 16;

        static constexpr ModeType ReadOnly    = Read;
        static constexpr ModeType ReadWrite   = Read | Write;
        static constexpr ModeType CreateWrite = Read | Write | Create;
        static constexpr ModeType Overwrite   = Read | Write | Create | Truncate;

    private:
        // Using heap allocation to keep states_ in a fixed address.
        std::unique_ptr<core::FileStates> pstates_;
        Group                             group_;

    public:

        File(const std::filesystem::path& path, ModeType mode) {
            open(path, mode);
        }

        ~File() {
            close();
        }

        File(File&&) = default;

        void open(const std::filesystem::path& path, ModeType mode) {
            close();

            // Set pstates_.
            pstates_ = std::make_unique<core::FileStates>();
            pstates_->open_state = (mode & Write) ? core::FileOpenState::ReadWrite : core::FileOpenState::ReadOnly;

            if(std::filesystem::is_directory(path)) {
                if(mode & Excl) {
                    throw Exception(path.string() + " already exists.");
                }
                else if(mode & Truncate) {
                    std::filesystem::remove_all(path);
                    group_ = Group(core::create_file_node(path), pstates_.get());
                }
                else {
                    group_ = Group(core::get_file_node(path), pstates_.get());
                }
            }
            else {
                if(mode & Create) {
                    group_ = Group(core::create_file_node(path), pstates_.get());
                }
                else {
                    throw Exception(path.string() + " does not exist.");
                }
            }
        }
        void close() {
            pstates_->open_state = core::FileOpenState::Closed;
        }

        // File as a group.
        //------------------------------
        auto load_attr() const { return group_.load_attr(); }
        auto save_attr(const Json& val) const { return group_.save_attr(val); }

        auto get_group(const std::filesystem::path& name) const { return group_.get_group(name); }
        auto create_group(const std::filesystem::path& name) const { return group_.create_group(name); }
        auto require_group(const std::filesystem::path& name) const { return group_.require_group(name); }
        void delete_group(const std::filesystem::path& name) const { return group_.delete_group(name); }

        auto get_dataset(const std::filesystem::path& name) const { return group_.get_dataset(name); }
        auto create_dataset(const std::filesystem::path& name) const { return group_.create_dataset(name); }
        auto require_dataset(const std::filesystem::path& name) const { return group_.require_dataset(name); }
        void delete_dataset(const std::filesystem::path& name) const { return group_.delete_dataset(name); }

    };

} // namespace poppel

#endif
