/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ContactCollector
*/

#include "plugins/physics/detail/ContactCollector.hpp"

#include <Jolt/Jolt.h>  // IWYU pragma: keep
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/Shape/SubShapeIDPair.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace rtype::plugins::physics::detail {
namespace {
constexpr std::uint32_t kHalfBits = 32U;
constexpr std::uint64_t kLowHalf = 0xFFFFFFFFU;

/// @return One key per pair of bodies, whatever their order: the smaller id in the high half.
std::uint64_t keyOf(JPH::BodyID first, JPH::BodyID second) noexcept {
    const std::uint64_t a = first.GetIndexAndSequenceNumber();
    const std::uint64_t b = second.GetIndexAndSequenceNumber();
    return a < b ? (a << kHalfBits) | b : (b << kHalfBits) | a;
}

/// @return The body in the high half of a key.
JPH::BodyID highBodyOf(std::uint64_t key) noexcept { return JPH::BodyID{static_cast<std::uint32_t>(key >> kHalfBits)}; }

/// @return The body in the low half of a key.
JPH::BodyID lowBodyOf(std::uint64_t key) noexcept { return JPH::BodyID{static_cast<std::uint32_t>(key & kLowHalf)}; }

/// @return Whether a moving body is asleep: Jolt removed its contacts, but it still touches what it touched.
bool isAsleep(const JPH::BodyInterface& bodies, JPH::BodyID body) {
    return bodies.IsAdded(body) && bodies.GetMotionType(body) != JPH::EMotionType::Static && !bodies.IsActive(body);
}

/// @return Whether a key names a pair that holds the body.
bool keyHolds(std::uint64_t key, JPH::BodyID body) noexcept {
    const std::uint64_t id = body.GetIndexAndSequenceNumber();
    return (key >> kHalfBits) == id || (key & kLowHalf) == id;
}
}  // namespace

void ContactCollector::OnContactAdded(const JPH::Body& first, const JPH::Body& second,
                                      const JPH::ContactManifold& manifold, JPH::ContactSettings& /*settings*/) {
    Pair& pair = _pairs[keyOf(first.GetID(), second.GetID())];
    if (pair.asleep) {
        pair.asleep = false;  // A body woke up: the contact never ended.
        ++pair.contacts;
        return;
    }
    if (pair.contacts++ > 0) {
        return;
    }
    const bool swap = !first.IsSensor() && second.IsSensor();
    pair.trigger = first.IsSensor() || second.IsSensor();
    pair.first = swap ? second.GetUserData() : first.GetUserData();
    pair.second = swap ? first.GetUserData() : second.GetUserData();
    _events.push_back(ContactEvent{
        .kind = pair.trigger ? ContactEvent::Kind::kTriggerEntered : ContactEvent::Kind::kCollisionStarted,
        .first = pair.first,
        .second = pair.second,
        .point = manifold.GetWorldSpaceContactPointOn1(0),
        .normal = swap ? -manifold.mWorldSpaceNormal : manifold.mWorldSpaceNormal,
    });
}

void ContactCollector::OnContactRemoved(const JPH::SubShapeIDPair& removed) {
    const std::uint64_t key = keyOf(removed.GetBody1ID(), removed.GetBody2ID());
    const auto found = _pairs.find(key);
    if (found != _pairs.end() && --found->second.contacts == 0) {
        _endings.push_back(key);  // The bodies are locked here: settle() tells an end from a body falling asleep.
    }
}

void ContactCollector::settle(const JPH::BodyInterface& bodies) {
    for (const std::uint64_t key : _endings) {
        const auto found = _pairs.find(key);
        if (found == _pairs.end() || found->second.contacts > 0) {
            continue;  // Forgotten with a destroyed body, or touching again within the same step.
        }
        Pair& pair = found->second;
        if (isAsleep(bodies, highBodyOf(key)) || isAsleep(bodies, lowBodyOf(key))) {
            pair.asleep = true;
            continue;
        }
        _events.push_back(ContactEvent{
            .kind = pair.trigger ? ContactEvent::Kind::kTriggerExited : ContactEvent::Kind::kCollisionEnded,
            .first = pair.first,
            .second = pair.second,
        });
        _pairs.erase(found);
    }
    _endings.clear();
}

std::vector<ContactCollector::ContactEvent> ContactCollector::take() { return std::exchange(_events, {}); }

void ContactCollector::forget(JPH::BodyID body) {
    std::erase_if(_pairs, [body](const auto& entry) { return keyHolds(entry.first, body); });
}

}  // namespace rtype::plugins::physics::detail
