/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EventReader
*/

#pragma once

#include <cstddef>

#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/event/EventRange.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/event/Events.hpp"

namespace rtype::ecs {
/// @brief An event type as a system parameter, read: the events of type `T` the system has not read yet.
///
/// @details Made at the start of each run from the world's events and the cursor the system keeps, it takes every
/// event sent since the system's previous run (still kept: the previous update's and the current one's) and moves the
/// cursor past them, so the system sees each event once whether it runs before or after the writer. It is a range of
/// `const T&`, invalidated by the next update or clear of the queue.
/// @code
/// void damage(rtype::ecs::EventReader<Collision> collisions, rtype::ecs::Commands& commands) {
///     for (const Collision& collision : collisions) {
///         commands.despawn(collision.b);
///     }
/// }
/// @endcode
template <Event T>
class EventReader {
  public:
    using Type = T;  ///< The event type.

    /// @param events The world's events.
    /// @param cursor The system's position in the queue of `T`, moved past the events taken.
    EventReader(const Events& events, EventCursor& cursor) noexcept;
    ~EventReader() = default;
    EventReader(const EventReader& other) noexcept = default;
    EventReader& operator=(const EventReader& other) noexcept = default;
    EventReader(EventReader&& other) noexcept = default;
    EventReader& operator=(EventReader&& other) noexcept = default;

    /// @return The number of events taken.
    [[nodiscard]] std::size_t size() const noexcept;

    /// @return Whether no event was sent since the system's previous run.
    [[nodiscard]] bool empty() const noexcept;

    /// @return An iterator at the oldest event taken.
    [[nodiscard]] EventRange<T>::Iterator begin() const noexcept;

    /// @return An iterator one past the newest event taken.
    [[nodiscard]] EventRange<T>::Iterator end() const noexcept;

  private:
    EventRange<T> _events;  ///< The events taken at the start of the run.
};
}  // namespace rtype::ecs

#include "EventReader.tpp"
