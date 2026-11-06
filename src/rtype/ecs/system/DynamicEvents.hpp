/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DynamicEvents
*/

#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/event/EventInfo.hpp"
#include "rtype/ecs/event/EventRange.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/system/SystemParam.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
/// @brief One event type a system uses, by id, and what it does with it.
struct EventUse {
    EventId id{kInvalidEventId};  ///< The event type, registered before the system is added.
    bool read{false};             ///< Whether the system reads the events (it gets a cursor in the queue).
    bool send{false};             ///< Whether the system sends events of this type.
};

/// @brief The configuration of a DynamicEvents parameter: the event types the system uses, given when it is added.
/// @details They are numbered in this order (DynamicEvents::read(0) reads the first).
struct EventList {
    std::vector<EventUse> events;  ///< The event types, in order.
};

/// @brief A system parameter reading and sending events chosen when the system is added, by id: for events the system
/// does not know at compile time (those a script names).
///
/// @details Configured with an EventList (see SystemParam): the system keeps one EventCursor per event type, as an
/// EventReader does, so it reads each event once, and its access set gets every type as read, or as written when it
/// sends (it then orders against the systems reading or sending the same type). Reading a type declared only to be
/// sent, or sending one declared only to be read, does nothing.
/// @code
/// void run(rtype::ecs::DynamicEvents events) {
///     for (const std::span<const std::byte> bytes : events.read(0)) {
///         // ...
///     }
/// }
/// app.addSystem(rtype::ecs::Phase::kUpdate, "game.read", &run, rtype::ecs::EventList{{{collisionId, true, false}}});
/// @endcode
class RTYPE_ECS_API DynamicEvents {
  public:
    /// @param events The world's events; they must outlive the view.
    /// @param uses The event types of the system, in order; they must outlive the view.
    /// @param cursors One cursor per use, in the same order; they must outlive the view.
    DynamicEvents(Events& events, std::span<const EventUse> uses, std::span<EventCursor> cursors) noexcept;

    ~DynamicEvents() = default;
    DynamicEvents(const DynamicEvents& other) noexcept = default;
    DynamicEvents& operator=(const DynamicEvents& other) noexcept = default;
    DynamicEvents(DynamicEvents&& other) noexcept = default;
    DynamicEvents& operator=(DynamicEvents&& other) noexcept = default;

    /// @return The number of event types, as given in the EventList.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @return The use at `index`; `index` must be below getCount().
    [[nodiscard]] const EventUse& getUse(std::size_t index) const noexcept;

    /// @brief Takes the events of the type at `index` that the system has not read yet.
    /// @return The unread events as raw bytes; empty if the type was not declared read. Invalidated by the next update
    /// of the queue.
    [[nodiscard]] RawEventRange read(std::size_t index) const noexcept;

    /// @brief Sends a copy of an event of the type at `index`.
    /// @param value The bytes of a live value of the event, exactly its size (empty for an event without data).
    /// @return False, sending nothing, if the type was not declared sent, `value` has the wrong size, or the event is
    /// not copyable.
    [[nodiscard]] bool send(std::size_t index, std::span<const std::byte> value) const;

  private:
    Events* _events;                  ///< The world's events.
    std::span<const EventUse> _uses;  ///< The system's event types.
    std::span<EventCursor> _cursors;  ///< Where the system is in each queue.
};

/// @brief `DynamicEvents`: configured with an EventList; the system keeps its cursors.
template <>
struct RTYPE_ECS_API SystemParam<DynamicEvents> {
    using Config = EventList;  ///< The event types to use, given when the system is added.

    /// @brief The event types and where the system is in each queue.
    struct State {
        std::vector<EventUse> uses;        ///< The event types, in order.
        std::vector<EventCursor> cursors;  ///< One cursor per use.
    };

    /// @return The list checked against the world, with a cursor at the start of each queue.
    /// @throws exceptions::TypeRegistrationException When an id is not a registered event.
    [[nodiscard]] static State init(World& world, Config config);

    /// @return A view of the events for this run.
    [[nodiscard]] static DynamicEvents get(State& state, const SystemContext& context) noexcept;

    /// @brief Adds reading, or sending, every event type of the list.
    static void addAccess(const State& state, SystemAccess& access);
};
}  // namespace rtype::ecs
