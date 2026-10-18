/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EventWriter
*/

#pragma once

#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/event/Events.hpp"

namespace rtype::ecs {
/// @brief An event type as a system parameter, written: sends events of type `T`.
/// @details Sending appends to the queue at once (events are not structural: no flush needed); readers see the event
/// in their next run, or later in this update if they run after this system.
/// @code
/// void detect(rtype::ecs::Query<const Position, rtype::ecs::With<Bullet>> bullets,
///             rtype::ecs::EventWriter<Collision> collisions) {
///     collisions.send(Collision{.a = bullet, .b = enemy});
/// }
/// @endcode
template <Event T>
class EventWriter {
  public:
    using Type = T;  ///< The event type.

    /// @param events The world's events.
    explicit EventWriter(Events& events) noexcept;
    ~EventWriter() = default;
    EventWriter(const EventWriter& other) noexcept = default;
    EventWriter& operator=(const EventWriter& other) noexcept = default;
    EventWriter(EventWriter&& other) noexcept = default;
    EventWriter& operator=(EventWriter&& other) noexcept = default;

    /// @brief Sends an event, moved in (Events::send()).
    void send(T event);

  private:
    Events* _events;  ///< The world's events.
};
}  // namespace rtype::ecs

#include "EventWriter.tpp"
