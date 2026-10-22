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
/// @details Registers RigidBody, Collider and Velocity, so they can be found by name (Luau, replication) before any
/// entity has them, and adds the TransformPlugin. Collider and RigidBody are replicated (ComponentOptions.hpp), through
/// ECS options only: the physics knows nothing of the network. The server simulates; clients receive the Transforms
/// and keep bodies that never step, only for scene queries (moveAndSlide() for prediction) and the debug view. In
/// standalone, everything runs as on the server. Each fixed tick, in this order:
/// - "rtype.Physics.SyncBodies", on every side, keeps one body per entity with a Collider and a Transform (a RigidBody
///   makes it move; without one it is static), rebuilt when its Collider or RigidBody changes, destroyed when the
///   entity is despawned or loses either, and linked to the entity by a PhysicsHandle;
/// - "rtype.Physics.SyncToPhysics", on every side, moves kinematic bodies to their Transform, teleports the others
///   gameplay moved and gives the bodies the Velocities gameplay changed (a kinematic body with a Velocity moves at
///   it); in client mode, it teleports every body to its Transform instead;
/// - "rtype.Physics.Step", on the server like those below, applies the forces and impulses queued on the PhysicsWorld
///   and steps it by Time::fixedDelta;
/// - "rtype.Physics.SyncFromPhysics" writes the pose of the bodies that moved back to their Transform, and the velocity
///   of the dynamic ones to their Velocity;
/// - "rtype.Physics.Events" sends the contacts of the step: CollisionStarted / CollisionEnded between solid colliders,
///   TriggerEntered / TriggerExited for sensors (CollisionEvents.hpp), registered by the plugin.
/// Other systems order themselves around these names. Needs the time plugin (MinimalPlugins) for Time.
/// @code
/// app.addPlugins(rtype::ecs::MinimalPlugins{})
///     .addPlugin(rtype::plugins::physics::PhysicsPlugin{{.gravity = {0.0F, 980.0F}}});
/// @endcode
class PhysicsPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.Physics";                         ///< The plugin's name.
    static constexpr std::string_view kSyncBodiesSystem = "rtype.Physics.SyncBodies";  ///< Creates and destroys bodies.
    static constexpr std::string_view kSyncToPhysicsSystem = "rtype.Physics.SyncToPhysics";  ///< Transform to body.
    static constexpr std::string_view kStepSystem = "rtype.Physics.Step";  ///< Steps the PhysicsWorld.
    static constexpr std::string_view kSyncFromPhysicsSystem = "rtype.Physics.SyncFromPhysics";  ///< Body to Transform.
    static constexpr std::string_view kEventsSystem = "rtype.Physics.Events";  ///< Sends the contact events.

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
