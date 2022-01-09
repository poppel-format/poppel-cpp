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

    class Dataset {
    private:
        core::Node          node_;
        core::FileStates*   pstates_ = nullptr;

    public:
        Dataset(core::Node node, core::FileStates* pstates):
            node_(std::move(node)), pstates_(pstates)
        {}

        //------------------------------
        // Accessors.
        //------------------------------
        auto filepath() const { return node_.path() / "data.npy"; }

        //------------------------------
        // Dataset operations.
        //------------------------------

        // Load only the header of the dataset.
        auto load_npy_header() const { return core::load_npy_header(filepath()); }

        // Load the data into the variable.
        // Most scalar types and common container types such as std::vector and std::string are supported.
        template< typename T >
        void load_to(T& val) const {
            core::assert_file_open(*pstates_);
            core::assert_is_node_dataset(node_);
            core::load_to(val, filepath());
        }

        // Load the data into the buffer, with the knowledge of data type, shape, and index order.
        //
        // If reshaping is not allowed, the data type, shape and index order must match exactly with the file, or an exception will be thrown.
        // If reshaping is allowed, the data type and total number of elements must match exactly with the file, or an exception will be thrown.
        // To preview the data type (word size), shape and index order, use load_npy_header().
        template< typename T >
        void load_to(T* val, bool fortran_order, std::vector<Size> shape, bool allow_reshape = false) const {
            core::assert_file_open(*pstates_);
            core::assert_is_node_dataset(node_);
            core::load_to(val, fortran_order, std::move(shape), filepath(), allow_reshape);
        }

        // Load the data into the buffer indicating 1D array, with the knowledge of data size.
        //
        // This function is simply the 1D special case to load_to() for multidimensional arrays.
        template< typename T >
        void load_to(T* val, Size size, bool allow_reshape = false) const {
            this->load_to(val, false, std::vector { size }, allow_reshape);
        }

        // Save the data from the variable.
        // Most scalar types and common container types such as std::vector and std::string are supported.
        template< typename T >
        void save_from(const T& val) const {
            core::assert_file_writable(*pstates_);
            core::assert_is_node_dataset(node_);
            core::save_from(val, filepath());
        }

        // Save the data using the buffer, with additional knowledge of data type, shape, and index order.
        template< typename T >
        void save_from(const T* val, bool fortran_order, std::vector<Size> shape) const {
            core::assert_file_writable(*pstates_);
            core::assert_is_node_dataset(node_);
            core::save_from(val, fortran_order, std::move(shape), filepath());
        }

        // Save 1D array from the buffer, with the knowledge of data size.
        template< typename T >
        void save_from(const T* val, Size size) const {
            this->save_from(val, false, std::vector { size });
        }

        // Attributes.
        auto load_attr() const {
            return core::load_attr(core::get_attribute(node_, *pstates_));
        }
        auto save_attr(const Json& val) const {
            return core::save_attr(val, core::get_attribute(node_, *pstates_));
        }
    };


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
        bool has_group(const std::filesystem::path& name) const;
        Group get_group(const std::filesystem::path& name) const;
        Group create_group(const std::filesystem::path& name) const;
        Group require_group(const std::filesystem::path& name) const;
        void delete_group(const std::filesystem::path& name) const;

        // Dataset management.
        bool has_dataset(const std::filesystem::path& name) const;
        Dataset get_dataset(const std::filesystem::path& name) const;
        // Create a dataset by passing args to Dataset::save_from function.
        template< typename... Args >
        Dataset create_dataset(const std::filesystem::path& name, Args&&... args) const {
            Dataset dataset(core::create_node(node_, name, *pstates_, core::NodeType::Dataset), pstates_);
            dataset.save_from(std::forward< Args >(args)...);
            return dataset;
        }
        // If the dataset does not exist, create it by passing args to Dataset::save_from function.
        template< typename... Args >
        Dataset require_dataset(const std::filesystem::path& name, Args&&... args) const {
            if(has_node(node_, name, *pstates_, core::NodeType::Dataset)) {
                return get_dataset(name);
            } else {
                return create_dataset(name, std::forward< Args >(args)...);
            }
        }
        void delete_dataset(const std::filesystem::path& name) const;

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

        File(const std::filesystem::path& path, ModeType mode) :
            pstates_(new core::FileStates { core::FileOpenState::Closed })
        {
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

        bool has_group(const std::filesystem::path& name) const { return group_.has_group(name); }
        auto get_group(const std::filesystem::path& name) const { return group_.get_group(name); }
        auto create_group(const std::filesystem::path& name) const { return group_.create_group(name); }
        auto require_group(const std::filesystem::path& name) const { return group_.require_group(name); }
        void delete_group(const std::filesystem::path& name) const { return group_.delete_group(name); }

        bool has_dataset(const std::filesystem::path& name) const { return group_.has_dataset(name); }
        auto get_dataset(const std::filesystem::path& name) const { return group_.get_dataset(name); }
        template< typename... Args >
        auto create_dataset(const std::filesystem::path& name, Args&&... args) const { return group_.create_dataset(name, std::forward<Args>(args)...); }
        template< typename... Args >
        auto require_dataset(const std::filesystem::path& name, Args&&... args) const { return group_.require_dataset(name, std::forward<Args>(args)...); }
        void delete_dataset(const std::filesystem::path& name) const { return group_.delete_dataset(name); }

    };

} // namespace poppel

#endif
