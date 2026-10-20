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
#include <Jolt/Physics/EPhysicsUpdateError.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <cstddef>
#include <cstdint>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <limits>
#include <memory>
#include <utility>

#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/detail/JoltMath.hpp"
#include "plugins/physics/detail/JoltRuntime.hpp"
#include "plugins/physics/detail/Layers.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"

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
}  // namespace

/// @brief The world's Jolt objects. Destroyed in reverse order: the system first, then what it points to.
struct PhysicsWorld::Impl {
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
    bool droppedContacts = false;                               ///< Whether the last step ignored contacts.
    JPH::TempAllocatorImpl tempAllocator;                       ///< Scratch memory for the steps.
    JPH::JobSystemSingleThreaded jobSystem;                     ///< Runs Jolt's jobs on the caller's thread.
    detail::BroadPhaseLayers broadPhaseLayers;                  ///< Object layer to broad phase layer.
    detail::ObjectVsBroadPhaseFilter objectVsBroadPhaseFilter;  ///< Object layer vs broad phase layer.
    detail::ObjectLayerPairFilter objectLayerPairFilter;        ///< Object layer vs object layer.
    JPH::PhysicsSystem system;                                  ///< Points to the three above: declared after them.
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

void PhysicsWorld::setGravity(glm::vec2 gravity) noexcept {
    _impl->system.SetGravity(detail::toJolt2D(gravity / _impl->unitsPerMeter));
}

void PhysicsWorld::step(float deltaSeconds) {
    const JPH::EPhysicsUpdateError errors =
        _impl->system.Update(deltaSeconds, kCollisionSteps, &_impl->tempAllocator, &_impl->jobSystem);
    _impl->droppedContacts = errors != JPH::EPhysicsUpdateError::None;
    ++_impl->steps;
}

}  // namespace rtype::plugins::physics
