/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Collider
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <type_traits>

#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/FieldTypeOf.hpp"

namespace rtype::plugins::physics {

/// @brief The shape of a collider.
enum class ShapeType : std::uint8_t {
    kBox,      ///< A rectangle of Collider::size.
    kCircle,   ///< A circle of Collider::radius.
    kCapsule,  ///< A vertical capsule: Collider::radius, and Collider::halfHeight for its straight part.
};

}  // namespace rtype::plugins::physics

// The ECS reflects scalars only: a ShapeType field is reflected as its underlying byte. The specialization must sit
// outside the physics namespace, in one that encloses rtype::ecs.
/// @brief A ShapeType field is reflected as kU8.
template <>
struct rtype::ecs::FieldTypeOf<rtype::plugins::physics::ShapeType>
    : std::integral_constant<rtype::ecs::FieldType, rtype::ecs::FieldType::kU8> {};
static_assert(sizeof(rtype::plugins::physics::ShapeType) == 1, "ShapeType must match FieldType::kU8");

namespace rtype::plugins::physics {

/// @brief Every collision category: a Collider::mask that collides with everything. There are 15 categories, bits 0 to
/// 14; each game names its own, e.g. `constexpr std::uint32_t kPlayerBullet = 1U << 1;`.
inline constexpr std::uint32_t kAllLayers = (1U << 15) - 1U;

/// @brief The shape and material an entity collides with: gameplay writes it, the physics plugin builds the shape.
/// @details Distances are in world units, like Transform. Each shape reads its own fields: size for a box, radius for
/// a circle, radius and halfHeight for a capsule. Without a RigidBody, the entity is a static body.
///
/// Collision filtering: two colliders collide (or a sensor detects the other) only when each one's layer is in the
/// other's mask, so both must want it; two static bodies never collide. layer must name at least one category, and
/// neither may use a bit beyond kAllLayers.
/// @code
/// commands.spawn()
///     .add(rtype::plugins::transform::Transform{.position = {400.0F, 300.0F}})
///     .add(rtype::plugins::physics::Collider{
///         .shape = rtype::plugins::physics::ShapeType::kBox,
///         .size = {800.0F, 16.0F},
///     });
/// @endcode
struct Collider {
    ShapeType shape = ShapeType::kBox;  ///< Which fields below describe the shape.
    glm::vec2 size{32.0F, 32.0F};       ///< kBox: full width and height, in world units.
    float radius = 16.0F;               ///< kCircle and kCapsule: radius, in world units.
    float halfHeight = 16.0F;           ///< kCapsule: half the length of the straight part, along Y, in world units.
    glm::vec2 offset{0.0F, 0.0F};       ///< The shape's center relative to the Transform, in world units, Y down.
    bool sensor = false;                ///< Detects overlaps (triggers) without colliding.
    float friction = 0.2F;              ///< How much the surface resists sliding, usually in [0, 1].
    float restitution = 0.0F;           ///< How bouncy the surface is: 0 absorbs, 1 bounces back fully.
    std::uint32_t layer = 1U << 0;      ///< The collision categories it belongs to, usually one bit.
    std::uint32_t mask = kAllLayers;    ///< The collision categories it collides with.
};
RTYPE_ECS_COMPONENT(Collider, "rtype.Collider", shape, size, radius, halfHeight, offset, sensor, friction, restitution,
                    layer, mask);

}  // namespace rtype::plugins::physics
