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
#include <Jolt/Math/Vec3.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/EActivation.h>
#include <Jolt/Physics/EPhysicsUpdateError.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <cstddef>
#include <cstdint>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <limits>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsHandle.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/detail/BodySettings.hpp"
#include "plugins/physics/detail/JoltMath.hpp"
#include "plugins/physics/detail/JoltRuntime.hpp"
#include "plugins/physics/detail/Layers.hpp"
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
}  // namespace

/// @brief The world's Jolt objects. Destroyed in reverse order: the system first, then what it points to.
struct PhysicsWorld::Impl {
    /// @brief What the world knows of an entity's body.
    struct BodyEntry {
        JPH::BodyID body;            ///< The body.
        bool hasRigidBody = false;   ///< Whether it was built with a RigidBody: catches one added or removed.
        std::uint64_t seenPass = 0;  ///< The last body pass that saw the entity.
    };

    explicit Impl(const PhysicsSettings& settings)
        : unitsPerMeter{settings.unitsPerMeter},
          tempAllocator{static_cast<JPH::uint>(settings.tempAllocatorBytes)},
          jobSystem{JPH::cMaxPhysicsJobs} {
        system.Init(settings.maxBodies, 0, settings.maxBodyPairs, settings.maxContactConstraints, broadPhaseLayers,
                    objectVsBroadPhaseFilter, objectLayerPairFilter);
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
    JPH::PhysicsSystem system;                                  ///< Points to the three above: declared after them.
    std::unordered_map<std::uint64_t, BodyEntry> bodies;        ///< The bodies by Entity::getBits().

    /// @brief Removes a body from the simulation and frees it.
    void destroy(JPH::BodyID id) {
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

std::optional<glm::vec2> PhysicsWorld::getBodyPosition(ecs::Entity entity) const {
    const auto found = _impl->bodies.find(entity.getBits());
    if (found == _impl->bodies.end()) {
        return std::nullopt;
    }
    return detail::fromJolt2D(_impl->system.GetBodyInterface().GetPosition(found->second.body)) * _impl->unitsPerMeter;
}

void PhysicsWorld::setGravity(glm::vec2 gravity) noexcept {
    _impl->system.SetGravity(detail::toJolt2D(gravity / _impl->unitsPerMeter));
}

void PhysicsWorld::step(float deltaSeconds) {
    const JPH::EPhysicsUpdateError errors =
        _impl->system.Update(deltaSeconds, kCollisionSteps, &_impl->tempAllocator, &_impl->jobSystem);
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

}  // namespace rtype::plugins::physics
