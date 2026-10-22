/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsHandle
*/

#pragma once

#include <cstdint>

#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::plugins::physics {

/// @brief Links an entity to its body in the PhysicsWorld: added and kept up to date by the physics plugin.
/// @details Internal: gameplay never writes it. The systems that work on bodies (sync, velocity, events) join on it to
/// reach the body without a lookup. A body id is only meaningful in its own world, so it is never replicated (the
/// default ReplicationMode::kNone).
struct PhysicsHandle {
    std::uint32_t body{0xffffffff};  ///< The Jolt body id (index and sequence number); all bits set when invalid.

    friend bool operator==(const PhysicsHandle& first, const PhysicsHandle& second) = default;
};
RTYPE_ECS_COMPONENT(PhysicsHandle, "rtype.PhysicsHandle", body);

}  // namespace rtype::plugins::physics
