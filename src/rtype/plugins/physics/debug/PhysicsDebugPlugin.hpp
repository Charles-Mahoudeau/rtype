/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsDebugPlugin
*/

#pragma once

#include <string_view>

#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::plugins::physics::debug {

/// @brief Draws the physics of chosen entities on the client: collider outlines and velocity arrows.
///
/// @details The PhysicsDebug component chooses the entities (as gizmos in Unity); PhysicsDebugSettings holds the global
/// switches: enabled (flipped by a key, F3 by default) and drawAll (every collider, PhysicsDebug or not). The plugin
/// does all the drawing, from the components (Collider, GlobalTransform, Velocity), so it shows exactly where sprites
/// are drawn and needs no physics body. Its systems run on the client side (so also in standalone):
/// - "rtype.PhysicsDebug.Toggle", in Update: flips PhysicsDebugSettings::enabled on the toggle key;
/// - "rtype.PhysicsDebug.Draw", in Render after the sprites: draws the outlines and arrows as shapes (shapes are
///   overlays, drawn on top of the sprites).
/// It draws through the BackendPlugin's renderer, so add it to clients with a window only; the headless server never
/// does. Registers PhysicsDebug, and inserts PhysicsDebugSettings if the App has none.
/// @code
/// app.addPlugin(rtype::plugins::backend::BackendPlugin{std::move(registry), settings})
///     .addPlugin(rtype::plugins::physics::debug::PhysicsDebugPlugin{});
/// @endcode
class PhysicsDebugPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.PhysicsDebug";                 ///< The plugin's name.
    static constexpr std::string_view kToggleSystem = "rtype.PhysicsDebug.Toggle";  ///< Flips enabled on the key.
    static constexpr std::string_view kDrawSystem = "rtype.PhysicsDebug.Draw";      ///< Draws the outlines and arrows.

    PhysicsDebugPlugin() = default;
    ~PhysicsDebugPlugin() override = default;
    PhysicsDebugPlugin(const PhysicsDebugPlugin& other) = default;
    PhysicsDebugPlugin& operator=(const PhysicsDebugPlugin& other) = default;
    PhysicsDebugPlugin(PhysicsDebugPlugin&& other) noexcept = default;
    PhysicsDebugPlugin& operator=(PhysicsDebugPlugin&& other) noexcept = default;

    /// @return "rtype.PhysicsDebug".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Registers the components and events it reads, inserts the settings, and adds the client systems.
    void build(ecs::App& app) override;
};

}  // namespace rtype::plugins::physics::debug
