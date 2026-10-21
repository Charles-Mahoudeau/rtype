/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ContactCollector
*/

#pragma once

#include <Jolt/Jolt.h>  // IWYU pragma: keep
#include <Jolt/Math/Real.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/Shape/SubShapeIDPair.h>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace rtype::plugins::physics::detail {

/// @brief Records the contacts Jolt reports during a step, as one start and one end per pair of bodies.
/// @details Jolt calls it during PhysicsSystem::Update(), on the thread that steps, since the world uses a
/// single-threaded job system: the callbacks fill plain containers, and the world takes the events after the step.
/// Nothing in the ECS can be touched from a callback. Jolt reports contacts per pair of sub-shapes, so each pair of
/// bodies counts them and only reports its first and last. A pair whose body is destroyed is forgotten without an end.
/// Jolt also removes the contacts of a body that falls asleep, and adds them back when it wakes up, while the bodies
/// still touch: the end of a pair is only decided by settle(), after the step, which keeps a sleeping pair silently.
class ContactCollector final : public JPH::ContactListener {
  public:
    /// @brief A contact that started or ended during the step, as Jolt reported it.
    struct ContactEvent {
        /// @brief What happened.
        enum class Kind : std::uint8_t {
            kCollisionStarted,  ///< Two solid bodies started touching.
            kCollisionEnded,    ///< They stopped.
            kTriggerEntered,    ///< A body entered a sensor.
            kTriggerExited,     ///< It left.
        };

        Kind kind = Kind::kCollisionStarted;     ///< What happened.
        std::uint64_t first = 0;                 ///< Entity::getBits() of the first body: the sensor, for a trigger.
        std::uint64_t second = 0;                ///< Entity::getBits() of the other body.
        JPH::RVec3 point = JPH::RVec3::sZero();  ///< Where they touched, in meters; zero for an end.
        JPH::Vec3 normal = JPH::Vec3::sZero();   ///< From first to second; zero for an end.
    };

    ContactCollector() = default;
    ~ContactCollector() override = default;
    ContactCollector(const ContactCollector& other) = delete;  // Jolt keeps a pointer to it: it never moves.
    ContactCollector& operator=(const ContactCollector& other) = delete;
    ContactCollector(ContactCollector&& other) noexcept = delete;
    ContactCollector& operator=(ContactCollector&& other) noexcept = delete;

    /// @brief Jolt: two sub-shapes started touching. Records a start when it is the pair's first contact.
    void OnContactAdded(const JPH::Body& first, const JPH::Body& second, const JPH::ContactManifold& manifold,
                        JPH::ContactSettings& settings) override;
    /// @brief Jolt: two sub-shapes stopped touching. When it was the pair's last contact, settle() decides the end.
    /// @details Jolt passes only body ids, whose bodies may be gone or locked: the pair remembers its entities.
    void OnContactRemoved(const JPH::SubShapeIDPair& removed) override;

    /// @brief Decides, after a step, the pairs whose last contact Jolt removed: a pair with a sleeping moving body
    /// still touches and stays, silently; the others end.
    /// @param bodies Read without locking: call it after PhysicsSystem::Update(), never during.
    void settle(const JPH::BodyInterface& bodies);
    /// @return The events recorded since the last call, emptying them; the pairs in contact stay.
    [[nodiscard]] std::vector<ContactEvent> take();
    /// @brief Forgets the pairs of a body about to be destroyed, so they never report an end: its entity is despawned,
    /// lost its collider, or is rebuilt with a new body.
    void forget(JPH::BodyID body);

  private:
    /// @brief A pair of bodies in contact, remembered while it lasts.
    struct Pair {
        std::uint64_t first = 0;     ///< Entity::getBits() of the first body: the sensor, for a trigger.
        std::uint64_t second = 0;    ///< Entity::getBits() of the other body.
        bool trigger = false;        ///< One of them is a sensor: a trigger, not a collision.
        std::uint32_t contacts = 0;  ///< Sub-shape contacts between them: the pair ends when it gets back to 0.
        bool asleep = false;         ///< Its contacts were removed because a body fell asleep: they come back.
    };

    std::vector<ContactEvent> _events;               ///< Filled by the callbacks, emptied by take().
    std::unordered_map<std::uint64_t, Pair> _pairs;  ///< The pairs in contact, by their two body ids packed.
    std::vector<std::uint64_t> _endings;             ///< Pairs whose last contact the step removed, for settle().
};

}  // namespace rtype::plugins::physics::detail
