/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RigidBody
*/

#pragma once

#include <cstdint>
#include <type_traits>

#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/FieldTypeOf.hpp"

namespace rtype::plugins::physics {

/// @brief How a body moves.
enum class BodyType : std::uint8_t {
    kStatic,     ///< Never moves (walls, ground).
    kKinematic,  ///< Moved by gameplay through its Transform, pushes dynamic bodies.
    kDynamic,    ///< Moved by the simulation: gravity, forces, collisions.
};

}  // namespace rtype::plugins::physics

// The ECS reflects scalars only: a BodyType field is reflected as its underlying byte. The specialization must sit
// outside the physics namespace, in one that encloses rtype::ecs.
/// @brief A BodyType field is reflected as kU8.
template <>
struct rtype::ecs::FieldTypeOf<rtype::plugins::physics::BodyType>
    : std::integral_constant<rtype::ecs::FieldType, rtype::ecs::FieldType::kU8> {};
static_assert(sizeof(rtype::plugins::physics::BodyType) == 1, "BodyType must match FieldType::kU8");

namespace rtype::plugins::physics {

/// @brief How an entity moves in the physics world: gameplay writes it, the physics plugin builds the body from it.
/// @details Plain data, no Jolt: it can be reflected, replicated and written from Luau. The entity also needs a
/// Collider to get a body; a Collider without a RigidBody is a static body.
/// @code
/// commands.spawn()
///     .add(rtype::plugins::transform::Transform{.position = {100.0F, 200.0F}})
///     .add(rtype::plugins::physics::RigidBody{.type = rtype::plugins::physics::BodyType::kDynamic})
///     .add(rtype::plugins::physics::Collider{.shape = rtype::plugins::physics::ShapeType::kCircle, .radius = 8.0F});
/// @endcode
struct RigidBody {
    BodyType type = BodyType::kDynamic;  ///< How the body moves.
    float mass = 1.0F;                   ///< In kilograms; only dynamic bodies use it.
    float linearDamping = 0.05F;         ///< Slows the linear velocity down over time, per second.
    float angularDamping = 0.05F;        ///< Slows the angular velocity down over time, per second.
    float gravityScale = 1.0F;           ///< Multiplies the world's gravity for this body: 0 ignores it.
    bool fixedRotation = false;          ///< Never rotates (a ship that must not spin on impact).
    bool continuousCollision = false;    ///< Never tunnels through thin colliders when fast (bullets), at a cost.
};
RTYPE_ECS_COMPONENT(RigidBody, "rtype.RigidBody", type, mass, linearDamping, angularDamping, gravityScale,
                    fixedRotation, continuousCollision);

}  // namespace rtype::plugins::physics
