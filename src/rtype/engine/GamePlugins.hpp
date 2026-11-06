/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GamePlugins
*/

#pragma once

#include <optional>
#include <string_view>

#include "Export.hpp"
#include "rtype/core/config/Settings.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/plugins/physics/PhysicsSettings.hpp"
#include "rtype/plugins/physics/debug/PhysicsDebugSettings.hpp"

namespace rtype::engine {
/// @brief The generic plugins a game lists in `project.luau`, with their settings, read and checked before the App
/// is built.
///
/// @details `plugins` is a list of names; the plugins the backend plugin adds on clients (input, sprite, text...) need
/// no name:
/// - `"transform"`: plugins::transform::TransformPlugin, for a server that places entities without physics;
/// - `"physics"`: plugins::physics::PhysicsPlugin, from the `physics` section (PhysicsSettings::fromSettings());
/// - `"physicsDebug"`: plugins::physics::debug::PhysicsDebugPlugin on the client side only (skipped on a server), from
///   the `physicsDebug` section (PhysicsDebugSettings::fromSettings()).
///
/// The `physics` and `physicsDebug` sections are checked on every side, listed or not, so a typo fails everywhere.
/// @code{.lua}
/// engine.settings.plugins = { "physics", "physicsDebug" }
/// engine.settings.physics.gravity = { 0, 0 }
/// engine.settings.physicsDebug.enabled = false
/// @endcode
struct RTYPE_ENGINE_API GamePlugins {
    static constexpr std::string_view kSettingsKey = "plugins";        ///< The list of names in the settings.
    static constexpr std::string_view kTransform = "transform";        ///< Names the TransformPlugin.
    static constexpr std::string_view kPhysics = "physics";            ///< Names the PhysicsPlugin, and its section.
    static constexpr std::string_view kPhysicsDebug = "physicsDebug";  ///< Names the PhysicsDebugPlugin, its section.

    bool transform{false};                                     ///< Adds the TransformPlugin.
    std::optional<plugins::physics::PhysicsSettings> physics;  ///< Adds the PhysicsPlugin with these settings.
    std::optional<plugins::physics::debug::PhysicsDebugSettings> physicsDebug;  ///< Adds the PhysicsDebugPlugin.

    /// @brief Reads the `plugins` list and the sections of the plugins it names.
    /// @param settings The project's settings.
    /// @param mode The mode the App runs in: `physicsDebug` is skipped when it has no client side.
    /// @return What to add.
    /// @throws exceptions::RuntimeError If a name is unknown.
    /// @throws core::config::exceptions::SettingsException If `plugins` is not a list of strings, or a section is
    /// invalid.
    [[nodiscard]] static GamePlugins fromSettings(const core::config::Settings& settings, ecs::RunMode mode);

    /// @brief Adds the plugins to @p app: the transform, then the physics, then the physics debug view, whose settings
    /// are inserted as its resource.
    void addTo(ecs::App& app) const;
};
}  // namespace rtype::engine
