/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsWorld
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <memory>
#include <optional>
#include <vector>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsHandle.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::physics {

namespace detail {
class JoltRuntime;
}  // namespace detail

/// @brief One physics world: a resource the PhysicsPlugin inserts, stepped once per fixed tick.
///
/// @details Each App (each room on a server) has its own world, stepped on the App's thread with no thread of its
/// own. Its API is in world units, Y down, like Transform; it converts to Jolt's meters
/// (PhysicsSettings::unitsPerMeter) and axes inside. The Jolt objects live behind a pointer, so moving the resource
/// never moves what Jolt points to, and this header includes no Jolt. A moved-from world can only be destroyed or
/// assigned to.
/// @code
/// void stop(rtype::ecs::ResMut<rtype::plugins::physics::PhysicsWorld> world) { world->setGravity({0.0F, 0.0F}); }
/// @endcode
class PhysicsWorld {
  public:
    /// @brief Where a body is, as a Transform sees it.
    struct BodyPose {
        ecs::Entity entity;  ///< The entity the body belongs to.
        glm::vec2 position;  ///< In world units, Y down.
        float rotation;      ///< In radians, clockwise on screen.
    };

    /// @brief Builds the world, setting Jolt up if this is the process's first world.
    /// @throws exceptions::PhysicsException When unitsPerMeter is not positive or a capacity is zero.
    explicit PhysicsWorld(const PhysicsSettings& settings);
    ~PhysicsWorld();
    PhysicsWorld(const PhysicsWorld& other) = delete;
    PhysicsWorld& operator=(const PhysicsWorld& other) = delete;
    PhysicsWorld(PhysicsWorld&& other) noexcept;
    PhysicsWorld& operator=(PhysicsWorld&& other) noexcept;

    /// @return The gravity, in world units per second squared, Y down.
    [[nodiscard]] glm::vec2 getGravity() const noexcept;
    /// @return World units in one of Jolt's meters.
    [[nodiscard]] float getUnitsPerMeter() const noexcept;
    /// @return The bodies in the world.
    [[nodiscard]] std::size_t getBodyCount() const noexcept;
    /// @return The steps done since the world was built.
    [[nodiscard]] std::uint64_t getStepCount() const noexcept;
    /// @return Whether the last step ignored contacts because a capacity (maxBodyPairs, maxContactConstraints) was
    /// reached: raise it in the PhysicsSettings.
    [[nodiscard]] bool didLastStepDropContacts() const noexcept;

    /// @return The entity's body, or nothing if it has none.
    [[nodiscard]] std::optional<PhysicsHandle> findBody(ecs::Entity entity) const;
    /// @return The entity a body belongs to, or nothing if the handle names no body of this world.
    [[nodiscard]] std::optional<ecs::Entity> findEntity(PhysicsHandle handle) const;
    /// @return Where the entity's body is, in world units, Y down; nothing if it has no body.
    [[nodiscard]] std::optional<glm::vec2> getBodyPosition(ecs::Entity entity) const;
    /// @return Whether the Transform differs from the last pose the world gave the body or read from it: gameplay moved
    /// the entity. False for an entity without a body.
    [[nodiscard]] bool isOutOfSync(ecs::Entity entity, const transform::Transform& transform) const;

    /// @param gravity In world units per second squared, Y down.
    void setGravity(glm::vec2 gravity) noexcept;

    /// @brief Advances the simulation by one step.
    /// @param deltaSeconds The step's duration; the PhysicsPlugin passes Time::fixedDelta.
    void step(float deltaSeconds);

    /// @brief Creates the entity's body from its components, replacing the one it had.
    /// @details A replaced moving body keeps its velocity. The body counts as seen by the current body pass.
    /// @param entity The entity, stored on the body to find it back from contacts.
    /// @param collider Its shape and material.
    /// @param body How it moves, or nullptr for a static body.
    /// @param at Where it is, in world units, Y down.
    /// @return The new body.
    /// @throws exceptions::PhysicsException When the components are invalid or the world holds maxBodies bodies.
    PhysicsHandle createBody(ecs::Entity entity, const Collider& collider, const RigidBody* body,
                             const transform::Transform& at);
    /// @brief Destroys the entity's body.
    /// @return Whether it had one.
    bool destroyBody(ecs::Entity entity);

    /// @brief Starts a body pass: the bodies that are not touched or created before endBodyPass() are destroyed.
    /// @details This is how bodies follow despawned entities and removed Colliders, which the ECS does not report: the
    /// physics plugin touches every entity that still has a Collider and a Transform, once per tick.
    void beginBodyPass() noexcept;
    /// @brief Marks the entity's body as seen by the current body pass.
    /// @param entity The entity, which still has a Collider and a Transform.
    /// @param hasRigidBody Whether it has a RigidBody now.
    /// @return False when its body must be created: it has none, or it gained or lost its RigidBody since.
    [[nodiscard]] bool touchBody(ecs::Entity entity, bool hasRigidBody) noexcept;
    /// @brief Ends the body pass: destroys the bodies it did not see.
    /// @return The entities whose body was destroyed.
    std::vector<ecs::Entity> endBodyPass();

    /// @brief Moves a kinematic body to the entity's Transform during the next step.
    /// @details Gives the body the velocity that reaches the Transform in deltaSeconds, so it pushes the dynamic bodies
    /// on its way instead of passing through them; called every tick, it stops when the Transform stops. Does nothing
    /// for an entity without a kinematic body.
    void moveKinematic(ecs::Entity entity, const transform::Transform& transform, float deltaSeconds);
    /// @brief Puts the entity's body at its Transform at once, waking it up: a respawn, a moved wall.
    /// @details Keeps the body's velocity. Does nothing for an entity without a body.
    void teleport(ecs::Entity entity, const transform::Transform& transform);
    /// @brief Reads the dynamic bodies the last step moved, for their Transforms.
    /// @details Only the bodies Jolt keeps active, and only those whose pose changed: sleeping bodies never mark their
    /// Transform as changed, so replication does not send them. Each pose is remembered, so isOutOfSync() does not take
    /// the Transform written from it for a move by gameplay.
    [[nodiscard]] std::vector<BodyPose> collectMovedBodies();

  private:
    struct Impl;

    std::shared_ptr<const detail::JoltRuntime> _runtime;  ///< Keeps Jolt set up while the world lives.
    std::unique_ptr<Impl> _impl;                          ///< The Jolt objects, at a stable address.
};
RTYPE_ECS_RESOURCE(PhysicsWorld, "rtype.PhysicsWorld");

}  // namespace rtype::plugins::physics
