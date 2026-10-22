/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsSettings
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>

namespace rtype::plugins::physics {

/// @brief How a physics world is built: given to the PhysicsPlugin, which builds the PhysicsWorld from it.
/// @details Distances are in world units (pixels today), like Transform: the world converts them to Jolt's meters with
/// unitsPerMeter, since Jolt's thresholds and margins are tuned for objects about a meter big.
/// @code
/// app.addPlugin(rtype::plugins::physics::PhysicsPlugin{{.gravity = {0.0F, 0.0F}, .unitsPerMeter = 32.0F}});
/// @endcode
struct PhysicsSettings {
    glm::vec2 gravity{0.0F, 0.0F};   ///< In world units per second squared, Y down. None: a shoot'em up has no gravity.
    float unitsPerMeter = 32.0F;     ///< World units in one of Jolt's meters; must be positive.
    std::uint32_t maxBodies = 4096;  ///< Bodies the world can hold.
    std::uint32_t maxBodyPairs = 4096;           ///< Body pairs the broad phase can report in one step.
    std::uint32_t maxContactConstraints = 2048;  ///< Contacts the solver can handle in one step.
    std::size_t tempAllocatorBytes = static_cast<std::size_t>(10) * 1024 * 1024;  ///< Scratch memory, per world.
};

}  // namespace rtype::plugins::physics
