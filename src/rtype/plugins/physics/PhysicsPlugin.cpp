/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsPlugin
*/

#include "plugins/physics/PhysicsPlugin.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/CollisionEvents.hpp"
#include "plugins/physics/ComponentOptions.hpp"
#include "plugins/physics/PhysicsCalls.hpp"
#include "plugins/physics/PhysicsHandle.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/Velocity.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformMath.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "plugins/transform/WorldPoses.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
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
using Bodies = ecs::Query<const Collider, ecs::Optional<const RigidBody>, const transform::Transform,
                          ecs::Optional<const ecs::Parent>, Filters...>;

/// @brief Everything that can be an ancestor of a body.
using Placed = ecs::Query<const transform::Transform, ecs::Optional<const ecs::Parent>>;

/// @brief The bodies that have a parent.
using ChildBodies = ecs::Query<const ecs::Parent, ecs::With<Collider>>;

/// @return Where the entities are in the world, to compute the pose of bodies with a parent. Empty when there is none:
/// a game without a hierarchy of bodies pays for nothing.
transform::WorldPoses collectPoses(Placed& placed, ChildBodies& children) {
    transform::WorldPoses poses;
    if (children.begin() == std::default_sentinel) {
        return poses;
    }
    for (auto [entity, local, parent] : placed) {
        poses.add(entity, local, parent);
    }
    return poses;
}

/// @return Where a body goes: its Transform, or for a child the pose its parents give it. Only the position and the
/// rotation are meaningful: bodies have no scale.
transform::Transform poseOf(const transform::WorldPoses& poses, ecs::Entity entity, const transform::Transform& local,
                            const ecs::Parent* parent) {
    if (parent == nullptr) {
        return local;
    }
    const transform::GlobalTransform pose = poses.of(entity);
    return transform::Transform{.position = pose.position, .rotation = pose.rotation, .scale = pose.scale};
}

/// @brief FixedUpdate, before the step: keeps one body per entity with a Collider and a Transform.
/// @details Rebuilds the bodies whose Collider or RigidBody changed (creating the new ones), creates the missing ones
/// and those whose RigidBody appeared or went, then destroys the bodies of entities that were despawned or lost their
/// Collider or Transform. Their PhysicsHandle follows, through the commands applied at the end of the phase.
void syncBodies(ecs::ResMut<PhysicsWorld> world, ecs::Commands& commands, Bodies<ecs::Changed<Collider>> colliders,
                ecs::Query<const Collider, const RigidBody, const transform::Transform,
                           ecs::Optional<const ecs::Parent>, ecs::Changed<RigidBody>>
                    bodies,
                Bodies<> all, ecs::Query<const PhysicsHandle> handles, Placed placed, ChildBodies children) {
    const transform::WorldPoses poses = collectPoses(placed, children);
    world->beginBodyPass();
    for (auto [entity, collider, body, transform, parent] : colliders) {
        commands.entity(entity).add(
            world->createBody(entity, collider, body, poseOf(poses, entity, transform, parent)));
    }
    for (auto [entity, collider, body, transform, parent] : bodies) {
        commands.entity(entity).add(
            world->createBody(entity, collider, &body, poseOf(poses, entity, transform, parent)));
    }
    for (auto [entity, collider, body, transform, parent] : all) {
        if (!world->touchBody(entity, body != nullptr)) {
            commands.entity(entity).add(
                world->createBody(entity, collider, body, poseOf(poses, entity, transform, parent)));
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
                              ecs::Optional<const ecs::Parent>, ecs::With<Collider>>
                       bodies,
                   Placed placed, ChildBodies children) {
    const transform::WorldPoses poses = collectPoses(placed, children);
    for (auto [entity, transform, body, velocity, parent] : bodies) {
        const transform::Transform pose = poseOf(poses, entity, transform, parent);
        const bool dynamicChild = parent != nullptr && body != nullptr && body->type == BodyType::kDynamic;
        const bool outOfSync =
            dynamicChild ? world->isLocalOutOfSync(entity, transform) : world->isOutOfSync(entity, pose);
        if (body != nullptr && body->type == BodyType::kKinematic && (velocity == nullptr || outOfSync)) {
            world->moveKinematic(entity, pose, time->fixedDelta);
            continue;
        }
        if (outOfSync) {
            world->teleport(entity, pose);
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
void followTransforms(
    ecs::ResMut<PhysicsWorld> world,
    ecs::Query<const transform::Transform, ecs::Optional<const ecs::Parent>, ecs::With<Collider>> bodies, Placed placed,
    ChildBodies children) {
    const transform::WorldPoses poses = collectPoses(placed, children);
    for (auto [entity, transform, parent] : bodies) {
        const transform::Transform pose = poseOf(poses, entity, transform, parent);
        if (world->isOutOfSync(entity, pose)) {
            world->teleport(entity, pose);
        }
    }
    world->releaseBroadPhase();
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

/// @brief A body with a parent whose Transform must be written back, once its parents' are.
struct ChildWrite {
    ecs::Entity entity;                    ///< The entity.
    ecs::Mut<transform::Transform> local;  ///< Its Transform, relative to its parent.
    ecs::Entity parent;                    ///< Its parent.
    glm::vec2 position;                    ///< Where its body is in the world.
    float rotation;                        ///< How its body is turned in the world.
};

/// @brief Writes the Transforms of `children`, parents first: each is the body's pose seen from its parent, so a
/// dynamic child stays in place when its parent moves. Records them, so the next tick does not take them for gameplay.
void writeChildren(PhysicsWorld& world, const transform::WorldPoses& poses, std::vector<ChildWrite>& children) {
    std::ranges::sort(children, {}, [&poses](const ChildWrite& child) { return poses.depthOf(child.entity); });
    for (ChildWrite& child : children) {
        const transform::GlobalTransform parent = poses.of(child.parent);
        const transform::Transform relative = transform::toLocal(
            parent, transform::GlobalTransform{.position = child.position, .rotation = child.rotation});
        const transform::Transform& current = child.local.get();
        if (current.position != relative.position || current.rotation != relative.rotation) {
            child.local->position = relative.position;
            child.local->rotation = relative.rotation;
        }
        world.rememberLocal(child.entity, child.local.get());
        const transform::GlobalTransform global = transform::combine(parent, child.local.get());
        world.rememberPose(child.entity,
                           transform::Transform{.position = global.position, .rotation = global.rotation});
    }
}

/// @brief FixedUpdate, after the step: the bodies the step moved write their pose to their Transform, and the dynamic
/// ones their velocity to their Velocity.
/// @details Only the moved bodies are written, so the Transforms of sleeping bodies are not marked as changed. Joins on
/// Collider rather than PhysicsHandle: a body created this tick gets its handle only when the phase ends, and its pose
/// must still be written, or the next tick would take the stale Transform for a teleport by gameplay. Dynamic children
/// are written every tick, moved or not: their parent may have moved under them.
void syncFromPhysics(ecs::ResMut<PhysicsWorld> world,
                     ecs::Query<transform::Transform, ecs::Optional<const ecs::Parent>, ecs::Optional<const RigidBody>,
                                ecs::With<Collider>>
                         transforms,
                     ecs::Query<Velocity, ecs::With<Collider>> velocities, Placed placed, ChildBodies children) {
    syncVelocitiesFromPhysics(*world, velocities);
    const std::vector<PhysicsWorld::BodyPose> moved = world->collectMovedBodies();
    const transform::WorldPoses ancestors = collectPoses(placed, children);
    if (moved.empty() && children.begin() == std::default_sentinel) {
        return;
    }
    std::unordered_map<std::uint64_t, const PhysicsWorld::BodyPose*> poses;
    poses.reserve(moved.size());
    for (const PhysicsWorld::BodyPose& pose : moved) {
        poses.emplace(pose.entity.getBits(), &pose);
    }
    std::vector<ChildWrite> writes;
    for (auto [entity, transform, parent, body] : transforms) {
        const auto found = poses.find(entity.getBits());
        if (parent != nullptr) {
            const bool dynamic = body != nullptr && body->type == BodyType::kDynamic;
            std::optional<PhysicsWorld::BodyPose> pose;
            if (dynamic) {
                pose = world->getBodyPose(entity);
            } else if (found != poses.end()) {
                pose = *found->second;
            }
            if (pose) {
                writes.push_back(ChildWrite{.entity = entity,
                                            .local = transform,
                                            .parent = parent->entity,
                                            .position = pose->position,
                                            .rotation = pose->rotation});
            }
        } else if (found != poses.end()) {
            transform->position = found->second->position;
            transform->rotation = found->second->rotation;
        }
    }
    writeChildren(*world, ancestors, writes);
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

std::span<const std::string_view> PhysicsPlugin::getDependencies() const noexcept {
    static constexpr std::array<std::string_view, 1> kDependencies{transform::TransformPlugin::kName};
    return kDependencies;
}

void PhysicsPlugin::build(ecs::App& app) {
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
    addPhysicsCalls(app);
}

}  // namespace rtype::plugins::physics
