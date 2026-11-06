/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsCalls
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <string_view>

#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep
#include "plugins/physics/Collider.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/call/CallTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::plugins::physics {

inline constexpr std::string_view kRaycastCall = "rtype.physics.raycast";            ///< PhysicsWorld::raycast().
inline constexpr std::string_view kShapeCastCall = "rtype.physics.shapeCast";        ///< PhysicsWorld::shapeCast().
inline constexpr std::string_view kOverlapCall = "rtype.physics.overlap";            ///< PhysicsWorld::overlap().
inline constexpr std::string_view kMoveAndSlideCall = "rtype.physics.moveAndSlide";  ///< moveAndSlide() an entity.
inline constexpr std::string_view kApplyForceCall = "rtype.physics.applyForce";      ///< PhysicsWorld::applyForce().
inline constexpr std::string_view kApplyImpulseCall = "rtype.physics.applyImpulse";  ///< PhysicsWorld::applyImpulse().
inline constexpr std::string_view kApplyTorqueCall = "rtype.physics.applyTorque";    ///< PhysicsWorld::applyTorque().
inline constexpr std::string_view kSetGravityCall = "rtype.physics.setGravity";      ///< PhysicsWorld::setGravity().

/// @brief The arguments of `rtype.physics.raycast`: a ray and the QueryFilter of the bodies it sees.
struct RaycastArguments {
    glm::vec2 origin{0.0F, 0.0F};     ///< Where the ray starts, in world units, Y down.
    glm::vec2 direction{1.0F, 0.0F};  ///< Where it goes, Y down; its length does not matter.
    float maxDistance{0.0F};          ///< How far it goes, in world units.
    std::uint32_t mask{kAllLayers};   ///< QueryFilter::mask: the categories it sees.
    ecs::Entity ignore;               ///< QueryFilter::ignore: an entity to skip; null skips none.
    bool includeSensors{false};       ///< QueryFilter::includeSensors.
};
RTYPE_ECS_CALL_DATA(RaycastArguments, "rtype.physics.RaycastArguments", origin, direction, maxDistance, mask, ignore,
                    includeSensors);

/// @brief The arguments of `rtype.physics.shapeCast`: a shape, named by Collider's shape fields, swept along a
/// direction.
struct ShapeCastArguments {
    ShapeType shape{ShapeType::kBox};  ///< Collider::shape: 0 a box, 1 a circle, 2 a capsule.
    glm::vec2 size{32.0F, 32.0F};      ///< Collider::size.
    float radius{16.0F};               ///< Collider::radius.
    float halfHeight{16.0F};           ///< Collider::halfHeight.
    glm::vec2 offset{0.0F, 0.0F};      ///< Collider::offset.
    glm::vec2 from{0.0F, 0.0F};        ///< Where the shape starts, in world units, Y down.
    glm::vec2 direction{1.0F, 0.0F};   ///< Where it goes, Y down; its length does not matter.
    float maxDistance{0.0F};           ///< How far it goes, in world units.
    std::uint32_t mask{kAllLayers};    ///< QueryFilter::mask.
    ecs::Entity ignore;                ///< QueryFilter::ignore; null skips none.
    bool includeSensors{false};        ///< QueryFilter::includeSensors.
};
RTYPE_ECS_CALL_DATA(ShapeCastArguments, "rtype.physics.ShapeCastArguments", shape, size, radius, halfHeight, offset,
                    from, direction, maxDistance, mask, ignore, includeSensors);

/// @brief The result of `rtype.physics.raycast` and `rtype.physics.shapeCast`: PhysicsWorld::RayHit, and whether
/// there is one.
struct HitResult {
    bool hit{false};               ///< Whether something was hit; the other fields are zero when not.
    ecs::Entity entity;            ///< The hit entity.
    glm::vec2 point{0.0F, 0.0F};   ///< Where, in world units, Y down.
    glm::vec2 normal{0.0F, 0.0F};  ///< The surface normal there, pointing out of the hit body, Y down.
    float distance{0.0F};          ///< How far the ray or the shape went before the hit, in world units.
};
RTYPE_ECS_CALL_DATA(HitResult, "rtype.physics.HitResult", hit, entity, point, normal, distance);

/// @brief The arguments of `rtype.physics.overlap`: a shape, named by Collider's shape fields, placed somewhere.
struct OverlapArguments {
    ShapeType shape{ShapeType::kBox};  ///< Collider::shape: 0 a box, 1 a circle, 2 a capsule.
    glm::vec2 size{32.0F, 32.0F};      ///< Collider::size.
    float radius{16.0F};               ///< Collider::radius.
    float halfHeight{16.0F};           ///< Collider::halfHeight.
    glm::vec2 offset{0.0F, 0.0F};      ///< Collider::offset.
    glm::vec2 at{0.0F, 0.0F};          ///< Where the shape is, in world units, Y down.
    std::uint32_t mask{kAllLayers};    ///< QueryFilter::mask.
    ecs::Entity ignore;                ///< QueryFilter::ignore; null skips none.
    bool includeSensors{false};        ///< QueryFilter::includeSensors.
};
RTYPE_ECS_CALL_DATA(OverlapArguments, "rtype.physics.OverlapArguments", shape, size, radius, halfHeight, offset, at,
                    mask, ignore, includeSensors);

/// @brief The result of `rtype.physics.overlap`: call data holds no list, so it gives how many entities overlap and
/// one of them.
struct OverlapResult {
    std::uint32_t count{0};  ///< How many entities overlap the shape.
    ecs::Entity first;       ///< One of them, null when there is none.
};
RTYPE_ECS_CALL_DATA(OverlapResult, "rtype.physics.OverlapResult", count, first);

/// @brief The arguments of `rtype.physics.moveAndSlide`: moves an entity's Collider, as moveAndSlide() with an entity.
struct MoveAndSlideArguments {
    ecs::Entity entity;           ///< The entity that moves: its Collider is the shape and its mask the filter.
    glm::vec2 from{0.0F, 0.0F};   ///< Where it starts (its Transform's position), in world units, Y down.
    glm::vec2 delta{0.0F, 0.0F};  ///< How far it would move without obstacles, in world units, Y down.
    float skin{0.5F};             ///< SlideSettings::skin.
    std::uint32_t maxSlides{4U};  ///< SlideSettings::maxSlides.
};
RTYPE_ECS_CALL_DATA(MoveAndSlideArguments, "rtype.physics.MoveAndSlideArguments", entity, from, delta, skin, maxSlides);

/// @brief The result of `rtype.physics.moveAndSlide`.
struct MoveAndSlideResult {
    glm::vec2 position{0.0F, 0.0F};  ///< Where the entity ends, in world units, Y down.
};
RTYPE_ECS_CALL_DATA(MoveAndSlideResult, "rtype.physics.MoveAndSlideResult", position);

/// @brief The arguments of `rtype.physics.applyForce` and `rtype.physics.applyImpulse`.
struct PushArguments {
    ecs::Entity entity;           ///< The entity, with a dynamic RigidBody.
    glm::vec2 value{0.0F, 0.0F};  ///< The force or the impulse, in world units, Y down.
};
RTYPE_ECS_CALL_DATA(PushArguments, "rtype.physics.PushArguments", entity, value);

/// @brief The arguments of `rtype.physics.applyTorque`.
struct TorqueArguments {
    ecs::Entity entity;  ///< The entity, with a dynamic RigidBody.
    float torque{0.0F};  ///< The torque, clockwise on screen.
};
RTYPE_ECS_CALL_DATA(TorqueArguments, "rtype.physics.TorqueArguments", entity, torque);

/// @brief The arguments of `rtype.physics.setGravity`.
struct GravityArguments {
    glm::vec2 gravity{0.0F, 0.0F};  ///< The new gravity, in world units per second squared, Y down.
};
RTYPE_ECS_CALL_DATA(GravityArguments, "rtype.physics.GravityArguments", gravity);

/// @brief Registers the physics engine calls on @p app, for scripts, on the `rtype.PhysicsWorld` resource: the scene
/// queries (`raycast`, `shapeCast`, `overlap`), `moveAndSlide`, the body pushes (`applyForce`, `applyImpulse`,
/// `applyTorque`) and `setGravity`. A body's velocity and position are set by writing its Velocity and Transform.
/// Called by the PhysicsPlugin.
void addPhysicsCalls(ecs::App& app);

}  // namespace rtype::plugins::physics
