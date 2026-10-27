/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ProjectSettingsRuntime
*/

#pragma once

#include <memory>

#include "ProjectSettings.hpp"
#include "rtype/luau/Library.hpp"
#include "rtype/luau/Runtime.hpp"

namespace rtype::engine {
/// @brief A sandboxed Luau runtime that exposes `engine.settings.*` and gathers what the project script sets.
class ProjectSettingsRuntime {
  public:
    /// @brief Creates the runtime and registers the `engine` library.
    /// @throws std::runtime_error If the runtime or the library cannot be set up.
    ProjectSettingsRuntime();
    ~ProjectSettingsRuntime() = default;
    ProjectSettingsRuntime(const ProjectSettingsRuntime&) = delete;
    ProjectSettingsRuntime& operator=(const ProjectSettingsRuntime&) = delete;
    ProjectSettingsRuntime(ProjectSettingsRuntime&&) = delete;
    ProjectSettingsRuntime& operator=(ProjectSettingsRuntime&&) = delete;

    /// @return The Luau runtime the project script runs in.
    [[nodiscard]] luau::Runtime& runtime();
    /// @return A copy of the settings set so far by the script.
    [[nodiscard]] ProjectSettings result() const;

  private:
    /// @return The `engine` library, holding the `settings` sub-library.
    [[nodiscard]] luau::Library buildLibrary();
    /// @return The `settings` library, whose setters write into `_result`.
    [[nodiscard]] luau::Library buildSettingsLibrary();

    luau::Runtime _runtime;                                                         ///< Runs the project script.
    std::shared_ptr<ProjectSettings> _result{std::make_shared<ProjectSettings>()};  ///< Shared with the setters.
    luau::Library _library;  ///< The registered `engine` library.
};
}  // namespace rtype::engine
