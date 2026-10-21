/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsWorld
*/

#include "plugins/physics/PhysicsWorld.hpp"

#include <Jolt/Jolt.h>  // IWYU pragma: keep
#include <Jolt/Core/Core.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Math/MathTypes.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/BodyManager.h>
#include <Jolt/Physics/Body/BodyType.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/EActivation.h>
#include <Jolt/Physics/EPhysicsUpdateError.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <glm/geometric.hpp>
#include <limits>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsHandle.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/QueryFilter.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/Velocity.hpp"
#include "plugins/physics/detail/BodySettings.hpp"
#include "plugins/physics/detail/ContactCollector.hpp"
#include "plugins/physics/detail/JoltMath.hpp"
#include "plugins/physics/detail/JoltRuntime.hpp"
#include "plugins/physics/detail/Layers.hpp"
#include "plugins/physics/detail/QueryFilters.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::plugins::physics {

namespace {
/// @brief One collision step per update: our fixed step (1/60 s) is short enough.
constexpr int kCollisionSteps = 1;

/// @brief Checks the settings, then takes a handle to Jolt's setup, so invalid settings never set Jolt up.
/// @throws exceptions::PhysicsException When a setting is invalid.
std::shared_ptr<const detail::JoltRuntime> acquireRuntime(const PhysicsSettings& settings) {
    if (!(settings.unitsPerMeter > 0.0F)) {
        throw exceptions::PhysicsException{
            std::format("PhysicsSettings::unitsPerMeter must be positive, got {}", settings.unitsPerMeter)};
    }
    if (settings.maxBodies == 0 || settings.maxBodyPairs == 0 || settings.maxContactConstraints == 0) {
        throw exceptions::PhysicsException{
            "PhysicsSettings: maxBodies, maxBodyPairs and maxContactConstraints "
            "must be positive"};
    }
    if (settings.tempAllocatorBytes == 0 || settings.tempAllocatorBytes > std::numeric_limits<JPH::uint>::max()) {
        throw exceptions::PhysicsException{std::format("PhysicsSettings::tempAllocatorBytes must be in [1, {}], got {}",
                                                       std::numeric_limits<JPH::uint>::max(),
                                                       settings.tempAllocatorBytes)};
    }
    return detail::JoltRuntime::acquire();
}

/// @return The entity whose Entity::getBits() gave these bits.
ecs::Entity entityFromBits(std::uint64_t bits) noexcept {
    return ecs::Entity{static_cast<std::uint32_t>(bits), static_cast<std::uint32_t>(bits >> 32U)};
}

/// @return A body's velocity in world units, Y down.
Velocity fromJoltVelocity(JPH::Vec3Arg linear, JPH::Vec3Arg angular, float unitsPerMeter) noexcept {
    return Velocity{.linear = detail::fromJolt2D(linear) * unitsPerMeter,
                    .angular = detail::fromJoltAngle2D(angular.GetZ())};
}
}  // namespace

/// @brief The world's Jolt objects. Destroyed in reverse order: the system first, then what it points to.
// The member order is what keeps destruction safe, so it is not reordered to save the padding (a few dozen bytes per
// world): the field order clang-analyzer suggests would destroy the layer filters before the system that uses them.
// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
struct PhysicsWorld::Impl {
    /// @brief What the world knows of an entity's body.
    struct BodyEntry {
        JPH::BodyID body;                    ///< The body.
        bool hasRigidBody = false;           ///< Whether it was built with a RigidBody: catches one added or removed.
        std::uint64_t seenPass = 0;          ///< The last body pass that saw the entity.
        glm::vec2 lastPosition{0.0F, 0.0F};  ///< The last position given to the body or read from it, in world units.
        float lastRotation = 0.0F;           ///< The last rotation given to the body or read from it.
        std::optional<Velocity>
            lastVelocity;                     ///< The last velocity given to the body or read from it; none if unknown.
        std::uint64_t kinematicMoveStep = 0;  ///< The step (as getStepCount() counts it after it) of the last
                                              ///< moveKinematic(): its pose is the Transform's, not written back.

        /// @return Whether this is the last pose given or read. Exact float equality on purpose: the Transform holds
        /// exactly the floats read from the body.
        [[nodiscard]] bool isAt(glm::vec2 position, float rotation) const noexcept {
            return lastPosition == position && lastRotation == rotation;
        }
        /// @brief Records the pose given to the body or read from it.
        void remember(glm::vec2 position, float rotation) noexcept {
            lastPosition = position;
            lastRotation = rotation;
        }
    };

    /// @brief What a push does to a body.
    enum class PushKind : std::uint8_t {
        kForce,    ///< JPH::BodyInterface::AddForce.
        kImpulse,  ///< JPH::BodyInterface::AddImpulse.
        kTorque,   ///< JPH::BodyInterface::AddTorque.
    };
    /// @brief A force, impulse or torque queued for the next step.
    struct Push {
        std::uint64_t entity;  ///< Entity::getBits() of the pushed entity.
        PushKind kind;         ///< What the push does.
        JPH::Vec3 value;       ///< Already in Jolt's units and axes.
    };

    explicit Impl(const PhysicsSettings& settings)
        : unitsPerMeter{settings.unitsPerMeter},
          tempAllocator{static_cast<JPH::uint>(settings.tempAllocatorBytes)},
          jobSystem{JPH::cMaxPhysicsJobs} {
        system.Init(settings.maxBodies, 0, settings.maxBodyPairs, settings.maxContactConstraints, broadPhaseLayers,
                    objectVsBroadPhaseFilter, objectLayerPairFilter);
        system.SetContactListener(&contacts);
        system.SetGravity(detail::toJolt2D(settings.gravity / unitsPerMeter));
    }
    ~Impl() = default;
    Impl(const Impl& other) = delete;
    Impl& operator=(const Impl& other) = delete;
    Impl(Impl&& other) noexcept = delete;
    Impl& operator=(Impl&& other) noexcept = delete;

    float unitsPerMeter;                                        ///< World units in one meter.
    std::uint64_t steps = 0;                                    ///< Steps done.
    std::uint64_t pass = 0;                                     ///< The current body pass.
    bool droppedContacts = false;                               ///< Whether the last step ignored contacts.
    JPH::TempAllocatorImpl tempAllocator;                       ///< Scratch memory for the steps.
    JPH::JobSystemSingleThreaded jobSystem;                     ///< Runs Jolt's jobs on the caller's thread.
    detail::BroadPhaseLayers broadPhaseLayers;                  ///< Object layer to broad phase layer.
    detail::ObjectVsBroadPhaseFilter objectVsBroadPhaseFilter;  ///< Object layer vs broad phase layer.
    detail::ObjectLayerPairFilter objectLayerPairFilter;        ///< Object layer vs object layer.
    detail::ContactCollector contacts;                          ///< Records the contacts during the steps.
    JPH::PhysicsSystem system;                                  ///< Points to the four above: declared after them.
    std::unordered_map<std::uint64_t, BodyEntry> bodies;        ///< The bodies by Entity::getBits().
    std::vector<Push> pushes;                                   ///< Queued for the next step, in call order.

    /// @brief Queues a push for the next step.
    void queue(ecs::Entity entity, PushKind kind, JPH::Vec3Arg value) {
        pushes.push_back(Push{.entity = entity.getBits(), .kind = kind, .value = value});
    }

    /// @brief Applies the queued pushes to the dynamic bodies they name, then forgets them.
    void applyPushes() {
        JPH::BodyInterface& bodyInterface = system.GetBodyInterface();
        for (const Push& push : pushes) {
            const auto found = bodies.find(push.entity);
            if (found == bodies.end() || bodyInterface.GetMotionType(found->second.body) != JPH::EMotionType::Dynamic) {
                continue;
            }
            switch (push.kind) {
                case PushKind::kForce:
                    bodyInterface.AddForce(found->second.body, push.value);
                    break;
                case PushKind::kImpulse:
                    bodyInterface.AddImpulse(found->second.body, push.value);
                    break;
                case PushKind::kTorque:
                    bodyInterface.AddTorque(found->second.body, push.value);
                    break;
            }
        }
        pushes.clear();
    }

    /// @return The body of the entity a query ignores; an invalid id when none is ignored or it has no body.
    [[nodiscard]] JPH::BodyID ignoredBody(const QueryFilter& filter) const {
        if (!filter.ignore) {
            return JPH::BodyID{};
        }
        const auto found = bodies.find(filter.ignore->getBits());
        return found == bodies.end() ? JPH::BodyID{} : found->second.body;
    }

    /// @return What a query hit, in world units: the entity reads the body's user data, the normal (in meters, unit)
    /// only flips Y; nothing if the body went away.
    [[nodiscard]] std::optional<RayHit> makeHit(JPH::BodyID body, JPH::RVec3Arg point, JPH::Vec3Arg normal,
                                                float distance) const {
        const JPH::BodyLockRead lock{system.GetBodyLockInterface(), body};
        if (!lock.Succeeded()) {
            return std::nullopt;
        }
        return RayHit{.entity = entityFromBits(lock.GetBody().GetUserData()),
                      .point = detail::fromJolt2D(point) * unitsPerMeter,
                      .normal = detail::fromJolt2D(normal),
                      .distance = distance};
    }

    /// @brief Removes a body from the simulation and frees it.
    void destroy(JPH::BodyID id) {
        contacts.forget(id);
        JPH::BodyInterface& bodyInterface = system.GetBodyInterface();
        bodyInterface.RemoveBody(id);
        bodyInterface.DestroyBody(id);
    }
};

PhysicsWorld::PhysicsWorld(const PhysicsSettings& settings)
    : _runtime{acquireRuntime(settings)}, _impl{std::make_unique<Impl>(settings)} {}

PhysicsWorld::~PhysicsWorld() = default;

PhysicsWorld::PhysicsWorld(PhysicsWorld&& other) noexcept = default;

PhysicsWorld& PhysicsWorld::operator=(PhysicsWorld&& other) noexcept {
    _impl = std::move(other._impl);
    _runtime = std::move(other._runtime);
    return *this;
}

glm::vec2 PhysicsWorld::getGravity() const noexcept {
    return detail::fromJolt2D(_impl->system.GetGravity()) * _impl->unitsPerMeter;
}

float PhysicsWorld::getUnitsPerMeter() const noexcept { return _impl->unitsPerMeter; }

std::size_t PhysicsWorld::getBodyCount() const noexcept { return _impl->system.GetNumBodies(); }

std::uint64_t PhysicsWorld::getStepCount() const noexcept { return _impl->steps; }

bool PhysicsWorld::didLastStepDropContacts() const noexcept { return _impl->droppedContacts; }

std::optional<PhysicsHandle> PhysicsWorld::findBody(ecs::Entity entity) const {
    const auto found = _impl->bodies.find(entity.getBits());
    if (found == _impl->bodies.end()) {
        return std::nullopt;
    }
    return PhysicsHandle{.body = found->second.body.GetIndexAndSequenceNumber()};
}

std::optional<ecs::Entity> PhysicsWorld::findEntity(PhysicsHandle handle) const {
    const JPH::BodyID id{handle.body};
    if (id.IsInvalid()) {
        return std::nullopt;
    }
    const JPH::BodyLockRead lock{_impl->system.GetBodyLockInterface(), id};
    if (!lock.Succeeded()) {
        return std::nullopt;
    }
    return entityFromBits(lock.GetBody().GetUserData());
}

bool PhysicsWorld::isOutOfSync(ecs::Entity entity, const transform::Transform& transform) const {
    const auto found = _impl->bodies.find(entity.getBits());
    return found != _impl->bodies.end() && !found->second.isAt(transform.position, transform.rotation);
}

std::optional<glm::vec2> PhysicsWorld::getBodyPosition(ecs::Entity entity) const {
    const auto found = _impl->bodies.find(entity.getBits());
    if (found == _impl->bodies.end()) {
        return std::nullopt;
    }
    return detail::fromJolt2D(_impl->system.GetBodyInterface().GetPosition(found->second.body)) * _impl->unitsPerMeter;
}

std::optional<Velocity> PhysicsWorld::getBodyVelocity(ecs::Entity entity) const {
    const auto found = _impl->bodies.find(entity.getBits());
    if (found == _impl->bodies.end()) {
        return std::nullopt;
    }
    const JPH::BodyInterface& bodyInterface = _impl->system.GetBodyInterface();
    return fromJoltVelocity(bodyInterface.GetLinearVelocity(found->second.body),
                            bodyInterface.GetAngularVelocity(found->second.body), _impl->unitsPerMeter);
}

bool PhysicsWorld::isVelocityOutOfSync(ecs::Entity entity, const Velocity& velocity) const {
    const auto found = _impl->bodies.find(entity.getBits());
    return found != _impl->bodies.end() &&
           _impl->system.GetBodyInterface().GetMotionType(found->second.body) != JPH::EMotionType::Static &&
           found->second.lastVelocity != velocity;
}

void PhysicsWorld::setGravity(glm::vec2 gravity) noexcept {
    _impl->system.SetGravity(detail::toJolt2D(gravity / _impl->unitsPerMeter));
}

void PhysicsWorld::step(float deltaSeconds) {
    _impl->applyPushes();
    const JPH::EPhysicsUpdateError errors =
        _impl->system.Update(deltaSeconds, kCollisionSteps, &_impl->tempAllocator, &_impl->jobSystem);
    _impl->contacts.settle(_impl->system.GetBodyInterfaceNoLock());
    _impl->droppedContacts = errors != JPH::EPhysicsUpdateError::None;
    ++_impl->steps;
}

PhysicsHandle PhysicsWorld::createBody(ecs::Entity entity, const Collider& collider, const RigidBody* body,
                                       const transform::Transform& at) {
    const JPH::BodyCreationSettings settings =
        detail::makeBodySettings(collider, body, at, _impl->unitsPerMeter, entity.getBits());
    JPH::BodyInterface& bodyInterface = _impl->system.GetBodyInterface();

    JPH::Vec3 linearVelocity = JPH::Vec3::sZero();
    JPH::Vec3 angularVelocity = JPH::Vec3::sZero();
    if (const auto old = _impl->bodies.find(entity.getBits()); old != _impl->bodies.end()) {
        if (bodyInterface.GetMotionType(old->second.body) != JPH::EMotionType::Static) {
            linearVelocity = bodyInterface.GetLinearVelocity(old->second.body);
            angularVelocity = bodyInterface.GetAngularVelocity(old->second.body);
        }
        _impl->destroy(old->second.body);
        _impl->bodies.erase(old);
    }

    const JPH::Body* const created = bodyInterface.CreateBody(settings);
    if (created == nullptr) {
        throw exceptions::PhysicsException{"The physics world is full: raise PhysicsSettings::maxBodies"};
    }
    const JPH::BodyID id = created->GetID();
    const bool moving = settings.mMotionType != JPH::EMotionType::Static;
    bodyInterface.AddBody(id, moving ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
    if (moving) {
        bodyInterface.SetLinearAndAngularVelocity(id, linearVelocity, angularVelocity);
    }
    _impl->bodies.insert_or_assign(entity.getBits(), Impl::BodyEntry{
                                                         .body = id,
                                                         .hasRigidBody = body != nullptr,
                                                         .seenPass = _impl->pass,
                                                         .lastPosition = at.position,
                                                         .lastRotation = at.rotation,
                                                     });
    return PhysicsHandle{.body = id.GetIndexAndSequenceNumber()};
}

bool PhysicsWorld::destroyBody(ecs::Entity entity) {
    const auto found = _impl->bodies.find(entity.getBits());
    if (found == _impl->bodies.end()) {
        return false;
    }
    _impl->destroy(found->second.body);
    _impl->bodies.erase(found);
    return true;
}

void PhysicsWorld::beginBodyPass() noexcept { ++_impl->pass; }

bool PhysicsWorld::touchBody(ecs::Entity entity, bool hasRigidBody) noexcept {
    const auto found = _impl->bodies.find(entity.getBits());
    if (found == _impl->bodies.end() || found->second.hasRigidBody != hasRigidBody) {
        return false;
    }
    found->second.seenPass = _impl->pass;
    return true;
}

std::vector<ecs::Entity> PhysicsWorld::endBodyPass() {
    std::vector<ecs::Entity> destroyed;
    for (auto entry = _impl->bodies.begin(); entry != _impl->bodies.end();) {
        if (entry->second.seenPass == _impl->pass) {
            ++entry;
            continue;
        }
        _impl->destroy(entry->second.body);
        destroyed.push_back(entityFromBits(entry->first));
        entry = _impl->bodies.erase(entry);
    }
    return destroyed;
}

void PhysicsWorld::moveKinematic(ecs::Entity entity, const transform::Transform& transform, float deltaSeconds) {
    const auto found = _impl->bodies.find(entity.getBits());
    if (found == _impl->bodies.end()) {
        return;
    }
    JPH::BodyInterface& bodyInterface = _impl->system.GetBodyInterface();
    if (bodyInterface.GetMotionType(found->second.body) != JPH::EMotionType::Kinematic) {
        return;
    }
    bodyInterface.MoveKinematic(found->second.body, detail::toJolt2D(transform.position / _impl->unitsPerMeter),
                                detail::toJoltRotation2D(transform.rotation), deltaSeconds);
    found->second.remember(transform.position, transform.rotation);
    found->second.lastVelocity.reset();
    found->second.kinematicMoveStep = _impl->steps + 1;
}

void PhysicsWorld::teleport(ecs::Entity entity, const transform::Transform& transform) {
    const auto found = _impl->bodies.find(entity.getBits());
    if (found == _impl->bodies.end()) {
        return;
    }
    _impl->system.GetBodyInterface().SetPositionAndRotation(
        found->second.body, detail::toJolt2D(transform.position / _impl->unitsPerMeter),
        detail::toJoltRotation2D(transform.rotation), JPH::EActivation::Activate);
    found->second.remember(transform.position, transform.rotation);
}

void PhysicsWorld::setVelocity(ecs::Entity entity, const Velocity& velocity) {
    const auto found = _impl->bodies.find(entity.getBits());
    if (found == _impl->bodies.end()) {
        return;
    }
    JPH::BodyInterface& bodyInterface = _impl->system.GetBodyInterface();
    if (bodyInterface.GetMotionType(found->second.body) == JPH::EMotionType::Static) {
        return;
    }
    bodyInterface.SetLinearAndAngularVelocity(found->second.body,
                                              detail::toJolt2D(velocity.linear / _impl->unitsPerMeter),
                                              JPH::Vec3{0.0F, 0.0F, detail::toJoltAngle2D(velocity.angular)});
    found->second.lastVelocity = velocity;
}

void PhysicsWorld::applyForce(ecs::Entity entity, glm::vec2 force) {
    _impl->queue(entity, Impl::PushKind::kForce, detail::toJolt2D(force / _impl->unitsPerMeter));
}

void PhysicsWorld::applyImpulse(ecs::Entity entity, glm::vec2 impulse) {
    _impl->queue(entity, Impl::PushKind::kImpulse, detail::toJolt2D(impulse / _impl->unitsPerMeter));
}

void PhysicsWorld::applyTorque(ecs::Entity entity, float torque) {
    const float metersSquared = _impl->unitsPerMeter * _impl->unitsPerMeter;
    _impl->queue(entity, Impl::PushKind::kTorque, JPH::Vec3{0.0F, 0.0F, detail::toJoltAngle2D(torque / metersSquared)});
}

std::vector<PhysicsWorld::BodyPose> PhysicsWorld::collectMovedBodies() {
    JPH::BodyIDVector active;
    _impl->system.GetActiveBodies(JPH::EBodyType::RigidBody, active);
    std::vector<BodyPose> poses;
    poses.reserve(active.size());
    const JPH::BodyLockInterface& locks = _impl->system.GetBodyLockInterface();
    for (const JPH::BodyID id : active) {
        const JPH::BodyLockRead lock{locks, id};
        if (!lock.Succeeded() || lock.GetBody().IsStatic()) {
            continue;
        }
        const JPH::Body& body = lock.GetBody();
        const auto found = _impl->bodies.find(body.GetUserData());
        if (found == _impl->bodies.end() || (body.IsKinematic() && found->second.kinematicMoveStep == _impl->steps)) {
            continue;
        }
        const glm::vec2 position = detail::fromJolt2D(body.GetPosition()) * _impl->unitsPerMeter;
        const float rotation = detail::fromJoltRotation2D(body.GetRotation());
        if (found->second.isAt(position, rotation)) {
            continue;
        }
        found->second.remember(position, rotation);
        poses.push_back(BodyPose{.entity = entityFromBits(found->first), .position = position, .rotation = rotation});
    }
    return poses;
}

// takeContacts() converts one kind to the other with a cast: both enums list the same kinds in the same order.
static_assert(static_cast<int>(PhysicsWorld::Contact::Kind::kCollisionStarted) ==
              static_cast<int>(detail::ContactCollector::ContactEvent::Kind::kCollisionStarted));
static_assert(static_cast<int>(PhysicsWorld::Contact::Kind::kCollisionEnded) ==
              static_cast<int>(detail::ContactCollector::ContactEvent::Kind::kCollisionEnded));
static_assert(static_cast<int>(PhysicsWorld::Contact::Kind::kTriggerEntered) ==
              static_cast<int>(detail::ContactCollector::ContactEvent::Kind::kTriggerEntered));
static_assert(static_cast<int>(PhysicsWorld::Contact::Kind::kTriggerExited) ==
              static_cast<int>(detail::ContactCollector::ContactEvent::Kind::kTriggerExited));

std::vector<PhysicsWorld::Contact> PhysicsWorld::takeContacts() {
    const std::vector<detail::ContactCollector::ContactEvent> events = _impl->contacts.take();
    std::vector<Contact> contacts;
    contacts.reserve(events.size());
    for (const detail::ContactCollector::ContactEvent& event : events) {
        contacts.push_back(Contact{
            .kind = static_cast<Contact::Kind>(event.kind),
            .first = entityFromBits(event.first),
            .second = entityFromBits(event.second),
            .point = detail::fromJolt2D(event.point) * _impl->unitsPerMeter,
            .normal = detail::fromJolt2D(event.normal),
        });
    }
    return contacts;
}

namespace {
/// @return The direction of a query, unit; nothing when it has no length or the query no reach.
std::optional<glm::vec2> queryDirection(glm::vec2 direction, float maxDistance) noexcept {
    const float length = glm::length(direction);
    if (!(length > 0.0F) || !(maxDistance > 0.0F)) {
        return std::nullopt;
    }
    return direction / length;
}
}  // namespace

std::optional<PhysicsWorld::RayHit> PhysicsWorld::raycast(glm::vec2 origin, glm::vec2 direction, float maxDistance,
                                                          const QueryFilter& filter) const {
    const std::optional<glm::vec2> unit = queryDirection(direction, maxDistance);
    if (!unit) {
        return std::nullopt;
    }
    const JPH::RRayCast ray{detail::toJolt2D(origin / _impl->unitsPerMeter),
                            detail::toJolt2D(*unit * (maxDistance / _impl->unitsPerMeter))};
    const detail::MaskLayerFilter layerFilter{filter.mask};
    const detail::QueryBodyFilter bodyFilter{_impl->ignoredBody(filter), filter.includeSensors};
    JPH::RayCastResult result;
    if (!_impl->system.GetNarrowPhaseQuery().CastRay(ray, result, {}, layerFilter, bodyFilter)) {
        return std::nullopt;
    }
    const JPH::RVec3 point = ray.GetPointOnRay(result.mFraction);
    JPH::Vec3 normal = JPH::Vec3::sZero();
    {
        // why the extra scope?
        // Because the lock must be released before makeHit() reads the body again to get its user data.
        const JPH::BodyLockRead lock{_impl->system.GetBodyLockInterface(), result.mBodyID};
        if (lock.Succeeded()) {
            normal = lock.GetBody().GetWorldSpaceSurfaceNormal(result.mSubShapeID2, point);
        }
    }
    return _impl->makeHit(result.mBodyID, point, normal, result.mFraction * maxDistance);
}

std::vector<PhysicsWorld::RayHit> PhysicsWorld::raycastAll(glm::vec2 origin, glm::vec2 direction, float maxDistance,
                                                           const QueryFilter& filter) const {
    const std::optional<glm::vec2> unit = queryDirection(direction, maxDistance);
    if (!unit) {
        return {};
    }
    const JPH::RRayCast ray{detail::toJolt2D(origin / _impl->unitsPerMeter),
                            detail::toJolt2D(*unit * (maxDistance / _impl->unitsPerMeter))};
    const detail::MaskLayerFilter layerFilter{filter.mask};
    const detail::QueryBodyFilter bodyFilter{_impl->ignoredBody(filter), filter.includeSensors};
    JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
    _impl->system.GetNarrowPhaseQuery().CastRay(ray, JPH::RayCastSettings{}, collector, {}, layerFilter, bodyFilter);
    collector.Sort();
    std::vector<RayHit> hits;
    hits.reserve(collector.mHits.size());
    std::vector<JPH::BodyID> seen;
    const JPH::BodyLockInterface& locks = _impl->system.GetBodyLockInterface();
    for (const JPH::RayCastResult& result : collector.mHits) {
        if (std::ranges::find(seen, result.mBodyID) != seen.end()) {
            continue;
        }
        seen.push_back(result.mBodyID);
        const JPH::RVec3 point = ray.GetPointOnRay(result.mFraction);
        JPH::Vec3 normal = JPH::Vec3::sZero();
        {
            // why the extra scope?
            // Because the lock must be released before makeHit() reads the body again to get its user data.
            const JPH::BodyLockRead lock{locks, result.mBodyID};
            if (lock.Succeeded()) {
                normal = lock.GetBody().GetWorldSpaceSurfaceNormal(result.mSubShapeID2, point);
            }
        }
        if (const std::optional<RayHit> hit =
                _impl->makeHit(result.mBodyID, point, normal, result.mFraction * maxDistance)) {
            hits.push_back(*hit);
        }
    }
    return hits;
}

std::optional<PhysicsWorld::RayHit> PhysicsWorld::shapeCast(const Collider& shape, glm::vec2 from, glm::vec2 direction,
                                                            float maxDistance, const QueryFilter& filter) const {
    const std::optional<glm::vec2> unit = queryDirection(direction, maxDistance);
    if (!unit) {
        return std::nullopt;
    }
    const JPH::RefConst<JPH::Shape> joltShape = detail::makeShape(shape, _impl->unitsPerMeter);
    const JPH::RShapeCast cast = JPH::RShapeCast::sFromWorldTransform(
        joltShape.GetPtr(), JPH::Vec3::sOne(), JPH::RMat44::sTranslation(detail::toJolt2D(from / _impl->unitsPerMeter)),
        detail::toJolt2D(*unit * (maxDistance / _impl->unitsPerMeter)));
    const detail::MaskLayerFilter layerFilter{filter.mask};
    const detail::QueryBodyFilter bodyFilter{_impl->ignoredBody(filter), filter.includeSensors};
    JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
    _impl->system.GetNarrowPhaseQuery().CastShape(cast, JPH::ShapeCastSettings{}, JPH::RVec3::sZero(), collector, {},
                                                  layerFilter, bodyFilter);
    if (!collector.HadHit()) {
        return std::nullopt;
    }
    const JPH::ShapeCastResult& result = collector.mHit;
    const JPH::Vec3 normal =
        result.mPenetrationAxis.IsNearZero() ? JPH::Vec3::sZero() : -result.mPenetrationAxis.Normalized();
    return _impl->makeHit(result.mBodyID2, JPH::RVec3{result.mContactPointOn2}, normal, result.mFraction * maxDistance);
}

std::vector<ecs::Entity> PhysicsWorld::overlap(const Collider& shape, glm::vec2 at, const QueryFilter& filter) const {
    const JPH::RefConst<JPH::Shape> joltShape = detail::makeShape(shape, _impl->unitsPerMeter);
    const JPH::RMat44 transform =
        JPH::RMat44::sTranslation(detail::toJolt2D(at / _impl->unitsPerMeter) + joltShape->GetCenterOfMass());
    const detail::MaskLayerFilter layerFilter{filter.mask};
    const detail::QueryBodyFilter bodyFilter{_impl->ignoredBody(filter), filter.includeSensors};
    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
    _impl->system.GetNarrowPhaseQuery().CollideShape(joltShape.GetPtr(), JPH::Vec3::sOne(), transform,
                                                     JPH::CollideShapeSettings{}, JPH::RVec3::sZero(), collector, {},
                                                     layerFilter, bodyFilter);
    std::vector<ecs::Entity> entities;
    for (const JPH::CollideShapeResult& result : collector.mHits) {
        const std::optional<ecs::Entity> entity =
            findEntity(PhysicsHandle{.body = result.mBodyID2.GetIndexAndSequenceNumber()});
        if (entity && std::ranges::find(entities, *entity) == entities.end()) {
            entities.push_back(*entity);
        }
    }
    return entities;
}

std::vector<PhysicsWorld::BodyVelocity> PhysicsWorld::collectChangedVelocities() {
    JPH::BodyIDVector active;
    _impl->system.GetActiveBodies(JPH::EBodyType::RigidBody, active);
    std::vector<BodyVelocity> velocities;
    velocities.reserve(active.size());
    const JPH::BodyLockInterface& locks = _impl->system.GetBodyLockInterface();
    for (const JPH::BodyID id : active) {
        const JPH::BodyLockRead lock{locks, id};
        if (!lock.Succeeded() || !lock.GetBody().IsDynamic()) {
            continue;
        }
        const JPH::Body& body = lock.GetBody();
        const auto found = _impl->bodies.find(body.GetUserData());
        if (found == _impl->bodies.end()) {
            continue;
        }
        const Velocity velocity =
            fromJoltVelocity(body.GetLinearVelocity(), body.GetAngularVelocity(), _impl->unitsPerMeter);
        if (found->second.lastVelocity == velocity) {
            continue;
        }
        found->second.lastVelocity = velocity;
        velocities.push_back(BodyVelocity{.entity = entityFromBits(found->first), .velocity = velocity});
    }
    return velocities;
}

}  // namespace rtype::plugins::physics
