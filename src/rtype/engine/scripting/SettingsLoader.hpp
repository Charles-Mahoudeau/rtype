/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SettingsLoader
*/

#pragma once

#include <filesystem>
#include <initializer_list>
#include <string>

#include "rtype/core/config/Settings.hpp"
#include "rtype/engine/Export.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/Table.hpp"

namespace rtype::engine::scripting {
/// @brief Reads the project settings from a Luau script (`game/project.luau`) into a core::config::Settings.
///
/// @details The script runs in a sandboxed runtime (no standard library) and sees one global, `engine`:
/// - `engine.<flag>`: the booleans given to setFlag() (`engine.isClient`, `engine.isDebug`...);
/// - `engine.settings`: the table the script fills, which build() flattens into dotted keys.
/// @code{.lua}
/// if engine.isClient then
///     engine.settings.renderer = "vulkan"
///     engine.settings.window.title = "R-Type"
///     engine.settings.window.size = { 800, 600 }
/// end
/// @endcode
/// gives `renderer`, `window.title` and `window.size`. A setting is a boolean, a number, a string, or an array of
/// numbers or of strings; tables nest. Intermediate tables a script assigns into must exist first: declare them with
/// declareGroups().
class RTYPE_ENGINE_API SettingsLoader {
  public:
    /// @brief Creates the sandboxed runtime and exposes the empty `engine` and `engine.settings` tables.
    /// @throws luau::exceptions::RuntimeError If the runtime cannot be created.
    SettingsLoader();
    ~SettingsLoader() = default;
    SettingsLoader(const SettingsLoader&) = delete;
    SettingsLoader& operator=(const SettingsLoader&) = delete;
    SettingsLoader(SettingsLoader&&) = delete;
    SettingsLoader& operator=(SettingsLoader&&) = delete;

    /// @brief Flattens `engine.settings` into a Settings.
    ///
    /// Can be called at any time, and again after more scripts are loaded: it reflects the table as it is.
    /// @return The settings the scripts have set; empty groups store nothing.
    /// @throws exceptions::RuntimeError If a value has an unsupported type: a function, a non-string key, an array
    /// mixed with named keys or with holes, or an array of mixed or other element types.
    [[nodiscard]] core::config::Settings build();

    /// @brief Exposes a boolean to scripts as `engine.<name>`, replacing the previous value.
    /// @param name The name of the flag.
    /// @param enabled Its value.
    /// @throws luau::exceptions::RuntimeError If the flag cannot be set.
    void setFlag(std::string_view name, bool enabled);

    /// @brief Creates empty tables under `engine.settings`, so a script can write `engine.settings.<group>.<key>`.
    /// @param groups The names of the groups; an existing group is replaced by an empty one.
    /// @throws luau::exceptions::RuntimeError If a group cannot be created.
    void declareGroups(std::initializer_list<std::string> groups);

    /// @brief Loads and runs a script, which fills `engine.settings`.
    /// @param path The Luau file to run.
    /// @throws luau::exceptions::RuntimeError If the file cannot be read or compiled, or if the script fails.
    void load(const std::filesystem::path& path);

  private:
    luau::Runtime _runtime;      ///< The sandboxed runtime the scripts run in.
    luau::Table _engineTable;    ///< The `engine` global.
    luau::Table _settingsTable;  ///< `engine.settings`, flattened by build().
};
}  // namespace rtype::engine::scripting
