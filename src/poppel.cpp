#include "poppel/core/operations.hpp"
#include "poppel/poppel.hpp"

namespace poppel {

    // Group management.
    Group Group::get_group(const std::filesystem::path& name) const {
        return Group(core::get_node(node_, name, *pstates_, core::NodeType::Group), pstates_);
    }
    Group Group::create_group(const std::filesystem::path& name) const {
        return Group(core::create_node(node_, name, *pstates_, core::NodeType::Group), pstates_);
    }
    Group Group::require_group(const std::filesystem::path& name) const {
        return Group(core::require_node(node_, name, *pstates_, core::NodeType::Group), pstates_);
    }
    void Group::delete_group(const std::filesystem::path& name) const {
        core::delete_node(node_, name, *pstates_);
    }

    // Parts of dataset management.
    Dataset Group::get_dataset(const std::filesystem::path& name) const {
        return Dataset(core::get_node(node_, name, *pstates_, core::NodeType::Dataset), pstates_);
    }
    void Group::delete_dataset(const std::filesystem::path& name) const {
        core::delete_node(node_, name, *pstates_);
    }

} // namespace poppel
