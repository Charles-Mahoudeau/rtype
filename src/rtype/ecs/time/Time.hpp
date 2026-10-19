/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Time
*/

#pragma once

#include <cstdint>

#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::ecs {
/// @brief The time of the frame and of the fixed ticks: a resource the scheduler inserts on its first frame and keeps
/// up to date (Scheduler::runFrame()).
/// @details Gameplay that must give the same result on every machine (movement, physics, network) runs in FixedUpdate
/// and uses fixedDelta; what follows the screen (camera, animation, UI) uses delta.
/// @code
/// void movement(rtype::ecs::Query<Position, const Velocity> query, rtype::ecs::Res<rtype::ecs::Time> time) {
///     for (auto [entity, position, velocity] : query) {
///         position->x += velocity.x * time->fixedDelta;
///     }
/// }
/// @endcode
struct Time {
    float delta{0.F};       ///< The last frame's duration, in seconds.
    float fixedDelta{0.F};  ///< The duration of one fixed tick, in seconds (1/60 by default).
    std::uint32_t tick{0};  ///< Fixed ticks completed since the start; while FixedUpdate runs, the current tick.
    float alpha{0.F};       ///< How far the frame is into the next fixed tick, in [0, 1): to interpolate what is drawn.
};
RTYPE_ECS_RESOURCE(Time, "rtype.Time", delta, fixedDelta, tick, alpha);
}  // namespace rtype::ecs
