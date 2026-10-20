/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsPlugin
*/

#include "plugins/physics/PhysicsPlugin.hpp"

#include <string>
#include <string_view>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsHandle.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/time/Time.hpp"

namespace rtype::plugins::physics {
namespace {
/// @brief Entities that get a body: a Collider and a Transform, maybe a RigidBody.
template <typename... Filters>
using Bodies = ecs::Query<const Collider, ecs::Optional<const RigidBody>, const transform::Transform, Filters...>;

/// @brief FixedUpdate, before the step: keeps one body per entity with a Collider and a Transform.
/// @details Rebuilds the bodies whose Collider or RigidBody changed (creating the new ones), creates the missing ones
/// and those whose RigidBody appeared or went, then destroys the bodies of entities that were despawned or lost their
/// Collider or Transform. Their PhysicsHandle follows, through the commands applied at the end of the phase.
void syncBodies(ecs::ResMut<PhysicsWorld> world, ecs::Commands& commands, Bodies<ecs::Changed<Collider>> colliders,
                ecs::Query<const Collider, const RigidBody, const transform::Transform, ecs::Changed<RigidBody>> bodies,
                Bodies<> all, ecs::Query<const PhysicsHandle> handles) {
    world->beginBodyPass();
    for (auto [entity, collider, body, transform] : colliders) {
        commands.entity(entity).add(world->createBody(entity, collider, body, transform));
    }
    for (auto [entity, collider, body, transform] : bodies) {
        commands.entity(entity).add(world->createBody(entity, collider, &body, transform));
    }
    for (auto [entity, collider, body, transform] : all) {
        if (!world->touchBody(entity, body != nullptr)) {
            commands.entity(entity).add(world->createBody(entity, collider, body, transform));
        }
    }
    static_cast<void>(world->endBodyPass());
    for (auto [entity, handle] : handles) {
        if (!world->findBody(entity)) {
            commands.entity(entity).remove<PhysicsHandle>();
        }
    }
}

/// @brief FixedUpdate: steps the world by one fixed tick.
void step(ecs::ResMut<PhysicsWorld> world, ecs::Res<ecs::Time> time) { world->step(time->fixedDelta); }
}  // namespace

PhysicsPlugin::PhysicsPlugin(PhysicsSettings settings) : _settings{settings} {}

PhysicsPlugin::PhysicsPlugin() : PhysicsPlugin{PhysicsSettings{}} {}

std::string_view PhysicsPlugin::getName() const noexcept { return kName; }

void PhysicsPlugin::build(ecs::App& app) {
    app.addPlugin(plugins::transform::TransformPlugin{});
    static_cast<void>(app.getWorld().getComponents().registerType<RigidBody>());
    static_cast<void>(app.getWorld().getComponents().registerType<Collider>());
    static_cast<void>(app.getWorld().getComponents().registerType<PhysicsHandle>());
    app.insertResource(PhysicsWorld{_settings});
    app.addSystem(ecs::Phase::kFixedUpdate, std::string{kSyncBodiesSystem}, &syncBodies);
    app.addSystem(ecs::Phase::kFixedUpdate, std::string{kStepSystem}, &step).after(std::string{kSyncBodiesSystem});
}

}  // namespace rtype::plugins::physics
