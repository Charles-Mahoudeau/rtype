/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsPlugin
*/

#include "plugins/physics/PhysicsPlugin.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/CollisionEvents.hpp"
#include "plugins/physics/ComponentOptions.hpp"
#include "plugins/physics/PhysicsHandle.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/Velocity.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/schedule/Schedules.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"
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

/// @brief FixedUpdate, after the body sync and before the step: gameplay's Transforms and Velocities drive the bodies.
/// @details Kinematic bodies without a Velocity move to their Transform every tick (and stop when it stops); those with
/// one move to their Transform on the ticks gameplay moved it, and at their Velocity otherwise. Other bodies only
/// follow a Transform that gameplay moved, as a teleport (respawn, moved wall): one the physics wrote itself is not a
/// move. Likewise, a Velocity is only given to the body when gameplay changed it.
void syncToPhysics(ecs::ResMut<PhysicsWorld> world, ecs::Res<ecs::Time> time,
                   ecs::Query<const transform::Transform, ecs::Optional<const RigidBody>, ecs::Optional<const Velocity>,
                              ecs::With<Collider>>
                       bodies) {
    for (auto [entity, transform, body, velocity] : bodies) {
        const bool outOfSync = world->isOutOfSync(entity, transform);
        if (body != nullptr && body->type == BodyType::kKinematic && (velocity == nullptr || outOfSync)) {
            world->moveKinematic(entity, transform, time->fixedDelta);
            continue;
        }
        if (outOfSync) {
            world->teleport(entity, transform);
        }
        if (velocity != nullptr && world->isVelocityOutOfSync(entity, *velocity)) {
            world->setVelocity(entity, *velocity);
        }
    }
}

/// @brief FixedUpdate, after the body sync, in client mode in place of syncToPhysics(): puts the bodies at their
/// Transform, which replication (or gameplay) wrote.
/// @details Clients never step, so their bodies only serve scene queries (predicted movement, lasers) and the debug
/// view: each one follows its Transform as a teleport, kinematic or not, and no Velocity is read.
void followTransforms(ecs::ResMut<PhysicsWorld> world,
                      ecs::Query<const transform::Transform, ecs::With<Collider>> bodies) {
    for (auto [entity, transform] : bodies) {
        if (world->isOutOfSync(entity, transform)) {
            world->teleport(entity, transform);
        }
    }
}

/// @brief FixedUpdate, after the step: the dynamic bodies write the velocity the step changed to their Velocity.
void syncVelocitiesFromPhysics(PhysicsWorld& world, ecs::Query<Velocity, ecs::With<Collider>>& velocities) {
    const std::vector<PhysicsWorld::BodyVelocity> changed = world.collectChangedVelocities();
    if (changed.empty()) {
        return;
    }
    std::unordered_map<std::uint64_t, const Velocity*> byEntity;
    byEntity.reserve(changed.size());
    for (const PhysicsWorld::BodyVelocity& body : changed) {
        byEntity.emplace(body.entity.getBits(), &body.velocity);
    }
    for (auto [entity, velocity] : velocities) {
        const auto found = byEntity.find(entity.getBits());
        if (found != byEntity.end()) {
            *velocity = *found->second;
        }
    }
}

/// @brief FixedUpdate, after the step: the bodies the step moved write their pose to their Transform, and the dynamic
/// ones their velocity to their Velocity.
/// @details Only the moved bodies are written, so the Transforms of sleeping bodies are not marked as changed. Joins on
/// Collider rather than PhysicsHandle: a body created this tick gets its handle only when the phase ends, and its pose
/// must still be written, or the next tick would take the stale Transform for a teleport by gameplay.
void syncFromPhysics(ecs::ResMut<PhysicsWorld> world, ecs::Query<transform::Transform, ecs::With<Collider>> transforms,
                     ecs::Query<Velocity, ecs::With<Collider>> velocities) {
    syncVelocitiesFromPhysics(*world, velocities);
    const std::vector<PhysicsWorld::BodyPose> moved = world->collectMovedBodies();
    if (moved.empty()) {
        return;
    }
    std::unordered_map<std::uint64_t, const PhysicsWorld::BodyPose*> poses;
    poses.reserve(moved.size());
    for (const PhysicsWorld::BodyPose& pose : moved) {
        poses.emplace(pose.entity.getBits(), &pose);
    }
    for (auto [entity, transform] : transforms) {
        const auto found = poses.find(entity.getBits());
        if (found != poses.end()) {
            transform->position = found->second->position;
            transform->rotation = found->second->rotation;
        }
    }
}

/// @brief FixedUpdate, after the write-back: sends the contacts the step reported as ECS events.
void sendContactEvents(ecs::ResMut<PhysicsWorld> world, ecs::EventWriter<CollisionStarted> started,
                       ecs::EventWriter<CollisionEnded> ended, ecs::EventWriter<TriggerEntered> entered,
                       ecs::EventWriter<TriggerExited> exited) {
    for (const PhysicsWorld::Contact& contact : world->takeContacts()) {
        switch (contact.kind) {
            case PhysicsWorld::Contact::Kind::kCollisionStarted:
                started.send(CollisionStarted{.first = contact.first,
                                              .second = contact.second,
                                              .point = contact.point,
                                              .normal = contact.normal});
                break;
            case PhysicsWorld::Contact::Kind::kCollisionEnded:
                ended.send(CollisionEnded{.first = contact.first, .second = contact.second});
                break;
            case PhysicsWorld::Contact::Kind::kTriggerEntered:
                entered.send(TriggerEntered{.trigger = contact.first, .other = contact.second});
                break;
            case PhysicsWorld::Contact::Kind::kTriggerExited:
                exited.send(TriggerExited{.trigger = contact.first, .other = contact.second});
                break;
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
    static_cast<void>(app.getWorld().getComponents().registerType<RigidBody>(kRigidBodyOptions));
    static_cast<void>(app.getWorld().getComponents().registerType<Collider>(kColliderOptions));
    static_cast<void>(app.getWorld().getComponents().registerType<Velocity>());
    static_cast<void>(app.getWorld().getComponents().registerType<PhysicsHandle>());
    ecs::Events& events = app.getWorld().getEvents();
    static_cast<void>(events.registerType<CollisionStarted>());
    static_cast<void>(events.registerType<CollisionEnded>());
    static_cast<void>(events.registerType<TriggerEntered>());
    static_cast<void>(events.registerType<TriggerExited>());
    app.insertResource(PhysicsWorld{_settings});
    app.addSystem(ecs::Phase::kFixedUpdate, std::string{kSyncBodiesSystem}, &syncBodies).setSide(ecs::Side::kBoth);
    // why the same name on a client?
    // Because systems ordered against it keep their place in every mode: a system of its own would be unordered
    // against them, and the schedule refuses that even in the modes where one of the two never runs.
    if (app.getSchedules().getRunMode() == ecs::RunMode::kClient) {
        app.addSystem(ecs::Phase::kFixedUpdate, std::string{kSyncToPhysicsSystem}, &followTransforms)
            .after(std::string{kSyncBodiesSystem})
            .setSide(ecs::Side::kClient);
    } else {
        app.addSystem(ecs::Phase::kFixedUpdate, std::string{kSyncToPhysicsSystem}, &syncToPhysics)
            .after(std::string{kSyncBodiesSystem});
    }
    app.addSystem(ecs::Phase::kFixedUpdate, std::string{kStepSystem}, &step).after(std::string{kSyncToPhysicsSystem});
    app.addSystem(ecs::Phase::kFixedUpdate, std::string{kSyncFromPhysicsSystem}, &syncFromPhysics)
        .after(std::string{kStepSystem});
    app.addSystem(ecs::Phase::kFixedUpdate, std::string{kEventsSystem}, &sendContactEvents)
        .after(std::string{kSyncFromPhysicsSystem});
}

}  // namespace rtype::plugins::physics
