/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentTicks
*/

#pragma once

#include "rtype/ecs/change/Tick.hpp"

namespace rtype::ecs {
/// @brief When one component value was added and last written.
/// @code
/// if (const std::optional<rtype::ecs::ComponentTicks> ticks = world.getTicks<Health>(ship);
///     ticks && ticks->isChanged(lastRun, world.getChangeTick())) {
///     // the ship's health changed since lastRun
/// }
/// @endcode
struct ComponentTicks {
    Tick added{0};    ///< Tick the value was added at.
    Tick changed{0};  ///< Tick the value was last written at; adding counts as writing.

    /// @return True if the value was added after `lastRun`, seen from `thisRun`.
    [[nodiscard]] constexpr bool isAdded(Tick lastRun, Tick thisRun) const noexcept {
        return isNewerThan(added, lastRun, thisRun);
    }

    /// @return True if the value was added or written after `lastRun`, seen from `thisRun`.
    [[nodiscard]] constexpr bool isChanged(Tick lastRun, Tick thisRun) const noexcept {
        return isNewerThan(changed, lastRun, thisRun);
    }
};
}  // namespace rtype::ecs
