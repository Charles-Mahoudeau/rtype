/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ChangeSet
*/

#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace rtype::core::reload {

/// @brief The files that changed since the last look, by their path relative to the watched root, in the portable form
/// of asset::normalizePath() (forward slashes). Each list is sorted.
struct ChangeSet {
    std::vector<std::string> added;     ///< Files that were not there before.
    std::vector<std::string> modified;  ///< Files that were written again.
    std::vector<std::string> removed;   ///< Files that are gone.

    /// @return Whether no file changed.
    [[nodiscard]] bool empty() const noexcept { return added.empty() && modified.empty() && removed.empty(); }

    /// @return How many files changed.
    [[nodiscard]] std::size_t size() const noexcept { return added.size() + modified.size() + removed.size(); }
};

}  // namespace rtype::core::reload
