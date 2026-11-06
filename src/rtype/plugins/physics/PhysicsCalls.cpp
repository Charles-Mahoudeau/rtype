/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsCalls
*/

#include "plugins/physics/PhysicsCalls.hpp"

#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <vector>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/MoveAndSlide.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/QueryFilter.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

namespace rtype::plugins::physics {
namespace {
/// @return The QueryFilter the call fields name; a null @p ignore skips no entity.
[[nodiscard]] QueryFilter filterOf(const std::uint32_t mask, const ecs::Entity ignore, const bool includeSensors) {
    QueryFilter filter{.mask = mask, .includeSensors = includeSensors};
    if (!ignore.isNull()) {
        filter.ignore = ignore;
    }
    return filter;
}

/// @return A Collider with the shape fields of call arguments, the others left at their defaults.
template <typename Arguments>
[[nodiscard]] Collider shapeOf(const Arguments& arguments) {
    return Collider{
        .shape = arguments.shape,
        .size = arguments.size,
        .radius = arguments.radius,
        .halfHeight = arguments.halfHeight,
        .offset = arguments.offset,
    };
}

/// @return The result of a ray or shape cast.
[[nodiscard]] HitResult resultOf(const std::optional<PhysicsWorld::RayHit>& hit) {
    if (!hit) {
        return HitResult{};
    }
    return HitResult{
        .hit = true,
        .entity = hit->entity,
        .point = hit->point,
        .normal = hit->normal,
        .distance = hit->distance,
    };
}

HitResult raycast(ecs::Res<PhysicsWorld> world, const RaycastArguments& arguments) {
    return resultOf(world->raycast(arguments.origin, arguments.direction, arguments.maxDistance,
                                   filterOf(arguments.mask, arguments.ignore, arguments.includeSensors)));
}

HitResult shapeCast(ecs::Res<PhysicsWorld> world, const ShapeCastArguments& arguments) {
    return resultOf(world->shapeCast(shapeOf(arguments), arguments.from, arguments.direction, arguments.maxDistance,
                                     filterOf(arguments.mask, arguments.ignore, arguments.includeSensors)));
}

OverlapResult overlap(ecs::Res<PhysicsWorld> world, const OverlapArguments& arguments) {
    const std::vector<ecs::Entity> entities = world->overlap(
        shapeOf(arguments), arguments.at, filterOf(arguments.mask, arguments.ignore, arguments.includeSensors));
    if (entities.empty()) {
        return OverlapResult{};
    }
    return OverlapResult{.count = static_cast<std::uint32_t>(entities.size()), .first = entities.front()};
}

MoveAndSlideResult slide(ecs::Res<PhysicsWorld> world, ecs::Query<const Collider> colliders,
                         const MoveAndSlideArguments& arguments) {
    // A Query has no lookup by entity: the colliders are walked until the one that moves.
    for (auto [entity, collider] : colliders) {
        if (entity == arguments.entity) {
            return MoveAndSlideResult{
                .position =
                    physics::moveAndSlide(*world, entity, collider, arguments.from, arguments.delta,
                                          SlideSettings{.skin = arguments.skin, .maxSlides = arguments.maxSlides}),
            };
        }
    }
    throw exceptions::PhysicsException{
        std::format("{}: entity {} has no Collider", kMoveAndSlideCall, arguments.entity.getIndex())};
}

void applyForce(ecs::ResMut<PhysicsWorld> world, const PushArguments& arguments) {
    world->applyForce(arguments.entity, arguments.value);
}

void applyImpulse(ecs::ResMut<PhysicsWorld> world, const PushArguments& arguments) {
    world->applyImpulse(arguments.entity, arguments.value);
}

void applyTorque(ecs::ResMut<PhysicsWorld> world, const TorqueArguments& arguments) {
    world->applyTorque(arguments.entity, arguments.torque);
}

void setGravity(ecs::ResMut<PhysicsWorld> world, const GravityArguments& arguments) {
    world->setGravity(arguments.gravity);
}
}  // namespace

void addPhysicsCalls(ecs::App& app) {
    app.addCall(std::string{kRaycastCall}, &raycast);
    app.addCall(std::string{kShapeCastCall}, &shapeCast);
    app.addCall(std::string{kOverlapCall}, &overlap);
    app.addCall(std::string{kMoveAndSlideCall}, &slide);
    app.addCall(std::string{kApplyForceCall}, &applyForce);
    app.addCall(std::string{kApplyImpulseCall}, &applyImpulse);
    app.addCall(std::string{kApplyTorqueCall}, &applyTorque);
    app.addCall(std::string{kSetGravityCall}, &setGravity);
}

}  // namespace rtype::plugins::physics
