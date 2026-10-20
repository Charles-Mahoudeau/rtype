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

#include "plugins/physics/PhysicsSettings.hpp"
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

    /// @param gravity In world units per second squared, Y down.
    void setGravity(glm::vec2 gravity) noexcept;

    /// @brief Advances the simulation by one step.
    /// @param deltaSeconds The step's duration; the PhysicsPlugin passes Time::fixedDelta.
    void step(float deltaSeconds);

  private:
    struct Impl;

    std::shared_ptr<const detail::JoltRuntime> _runtime;  ///< Keeps Jolt set up while the world lives.
    std::unique_ptr<Impl> _impl;                          ///< The Jolt objects, at a stable address.
};
RTYPE_ECS_RESOURCE(PhysicsWorld, "rtype.PhysicsWorld");

}  // namespace rtype::plugins::physics
