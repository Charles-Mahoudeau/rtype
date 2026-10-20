/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsPlugin
*/

#pragma once

#include <string_view>

#include "plugins/physics/PhysicsSettings.hpp"
#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::plugins::physics {

/// @brief Physics for an App: registers the physics components, inserts the PhysicsWorld resource and steps it once per
/// fixed tick.
///
/// @details Registers RigidBody and Collider, so they can be found by name (Luau, replication) before any entity has
/// them, and adds the TransformPlugin. In FixedUpdate, "rtype.Physics.SyncBodies" keeps one body per entity with a
/// Collider and a Transform (a RigidBody makes it move; without one it is static), rebuilt when its Collider or
/// RigidBody changes, destroyed when the entity is despawned or loses either, and linked to the entity by a
/// PhysicsHandle. Then a FixedUpdate system, "rtype.Physics.Step", which steps the world by Time::fixedDelta. It runs
/// on the server side (so also in standalone): clients receive positions through replication. The systems that sync the
/// ECS with the world order themselves around it with kStepSystem. Needs the time plugin (MinimalPlugins) for Time.
/// @code
/// app.addPlugins(rtype::ecs::MinimalPlugins{})
///     .addPlugin(rtype::plugins::physics::PhysicsPlugin{{.gravity = {0.0F, 980.0F}}});
/// @endcode
class PhysicsPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.Physics";                         ///< The plugin's name.
    static constexpr std::string_view kSyncBodiesSystem = "rtype.Physics.SyncBodies";  ///< Creates and destroys bodies.
    static constexpr std::string_view kStepSystem = "rtype.Physics.Step";              ///< Steps the PhysicsWorld.

    /// @param settings How the PhysicsWorld is built; checked when the plugin is built.
    explicit PhysicsPlugin(PhysicsSettings settings);
    PhysicsPlugin();
    ~PhysicsPlugin() override = default;
    PhysicsPlugin(const PhysicsPlugin& other) = default;
    PhysicsPlugin& operator=(const PhysicsPlugin& other) = default;
    PhysicsPlugin(PhysicsPlugin&& other) noexcept = default;
    PhysicsPlugin& operator=(PhysicsPlugin&& other) noexcept = default;

    /// @return "rtype.Physics".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Registers the components, inserts the PhysicsWorld and adds the FixedUpdate step system.
    /// @throws exceptions::PhysicsException When the settings are invalid.
    void build(ecs::App& app) override;

  private:
    PhysicsSettings _settings;  ///< Given to the PhysicsWorld.
};

}  // namespace rtype::plugins::physics
