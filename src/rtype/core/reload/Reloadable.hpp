/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Reloadable
*/

#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "ChangeSet.hpp"
#include "ReloadOutcome.hpp"

namespace rtype::core::reload {

/// @brief Where a Reloadable looks for files.
struct WatchedPath {
    std::filesystem::path path;           ///< A file or a folder (watched with its subfolders), relative to the root.
    std::vector<std::string> extensions;  ///< The extensions to follow in a folder (".luau"); empty: every file.
};

/// @brief A feature that can apply new versions of its files while the game runs: scripts, shaders, the project file.
///
/// @details A HotReloader watches what watched() lists and calls reload() with the files that changed. To make a new
/// feature hot reloadable, implement this and add it to the reloader: nothing else knows the feature.
class Reloadable {
  public:
    Reloadable() = default;
    virtual ~Reloadable() = default;
    Reloadable(const Reloadable& other) = default;
    Reloadable& operator=(const Reloadable& other) = default;
    Reloadable(Reloadable&& other) noexcept = default;
    Reloadable& operator=(Reloadable&& other) noexcept = default;

    /// @return The name the logs use for this feature ("scripts").
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    /// @return The files this feature follows. Read once, when the reloadable is added.
    [[nodiscard]] virtual std::vector<WatchedPath> watched() const = 0;

    /// @brief Applies the changed files. Called between two frames.
    /// @param changes The files of watched() that changed; never empty.
    /// @return What happened. An exception is caught by the reloader and reported as a failure.
    [[nodiscard]] virtual ReloadOutcome reload(const ChangeSet& changes) = 0;
};

}  // namespace rtype::core::reload
