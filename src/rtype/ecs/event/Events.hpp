/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Events
*/

#pragma once

#include <cstddef>
#include <deque>
#include <optional>
#include <span>
#include <string_view>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/event/EventInfo.hpp"
#include "rtype/ecs/event/EventRange.hpp"
#include "rtype/ecs/event/EventRegistry.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/storage/EventStorage.hpp"

namespace rtype::ecs {
namespace detail {
struct WorldState;
}  // namespace detail

/// @brief The events of a world: their registry and one double-buffered queue per event type, holding the messages
/// systems send to each other (a collision, a window resize, a client connecting).
///
/// @details Reached through World::getEvents(); it only exists inside a world. An event is readable during the update
/// it was sent in and the next one, then dropped: update() moves every queue forward, update(id) one queue (the
/// scheduler updates each queue with the phase that writes it, once per fixed tick for FixedUpdate events). Each
/// reader keeps an EventCursor, so it reads each event once. Events are local to a world and never replicated. Event
/// ids are per world, and only names are shared across worlds and processes.
/// @code
/// rtype::ecs::Events& events = world.getEvents();
/// events.send(Collision{.a = bullet, .b = enemy});
/// rtype::ecs::EventCursor cursor;
/// for (const Collision& collision : events.read<Collision>(cursor)) {
///     // ...
/// }
/// events.update();
/// @endcode
class RTYPE_ECS_API Events {
  public:
    /// @param state The state of the world these events belong to; it must outlive them.
    explicit Events(const detail::WorldState& state) noexcept;
    /// @brief Destroys every event kept.
    ~Events() = default;
    Events(const Events& other) = delete;
    Events& operator=(const Events& other) = delete;
    Events(Events&& other) = delete;
    Events& operator=(Events&& other) = delete;

    /// @return The events registered in this world.
    [[nodiscard]] const EventRegistry& getRegistry() const noexcept;

    /// @brief Registers a C++ event, or returns its id if it is already registered with the same options.
    /// @details send() registers the event itself; call this to know its id, or to register it before it is sent.
    /// @throws exceptions::TypeRegistrationException When the name is taken by another layout, other options, a
    /// component or a resource.
    template <Event T>
    EventId registerType(EventOptions options = {});

    /// @brief Registers an event described at run time (a Luau event), and creates its empty queue.
    /// @return Its id (the existing one if the same description was registered before), or why it was refused.
    [[nodiscard]] Result<EventId> registerType(EventInfo info);

    /// @return The id of the event with this name in this world, or nothing if it is not registered.
    [[nodiscard]] std::optional<EventId> find(std::string_view name) const noexcept;

    /// @brief Sends a copy of an event.
    /// @details Used by the Luau bridge and the network, which only know events by id. If copying throws, nothing is
    /// sent.
    /// @param value The bytes of a live value of the event, exactly its size (empty for an event without data).
    /// @return False, sending nothing, if the id is unknown, `value` has the wrong size, or the event is not copyable.
    bool send(EventId id, std::span<const std::byte> value);

    /// @brief Takes the events of the queue that `cursor` has not read yet, and moves `cursor` past them.
    /// @details Events older than the previous update are already dropped: a reader that skipped them does not see
    /// them. The range is invalidated by the next update or clear of the queue.
    /// @return The unread events as raw bytes; empty, leaving `cursor` as is, if the id is unknown.
    [[nodiscard]] RawEventRange read(EventId id, EventCursor& cursor) const noexcept;

    /// @return The number of events kept in the queue (those of the previous and of the current update); 0 for an
    /// unknown id.
    [[nodiscard]] std::size_t getCount(EventId id) const noexcept;

    /// @brief Drops the events of the queue's previous update and makes the current ones previous.
    /// @return False, doing nothing, if the id is unknown.
    bool update(EventId id) noexcept;

    /// @brief Drops every event of the queue. Cursors stay valid: they simply have nothing left to read.
    /// @return False, doing nothing, if the id is unknown.
    bool clear(EventId id) noexcept;

    /// @brief Moves every queue forward, as update(id) does: each event is kept for the update it was sent in and the
    /// next one.
    void update() noexcept;

    /// @brief Sends a C++ event.
    /// @details Registers `T` with the default options if it is not registered yet, then moves `value` in.
    /// @code
    /// world.getEvents().send(Collision{.a = bullet, .b = enemy});
    /// @endcode
    /// @throws exceptions::TypeRegistrationException When `T` is not registered and its name is already taken by
    /// another layout (a run-time event with the same name), a component or a resource.
    template <Event T>
    void send(T value);

    /// @brief Takes the events of type `T` that `cursor` has not read yet, and moves `cursor` past them.
    /// @return The unread events; empty, leaving `cursor` as is, if `T` is not registered. Invalidated by the next
    /// update or clear of the queue.
    template <Event T>
    [[nodiscard]] EventRange<T> read(EventCursor& cursor) const noexcept;

    /// @return The number of events of type `T` kept; 0 if `T` is not registered.
    template <Event T>
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @brief Moves the queue of `T` forward.
    /// @return False, doing nothing, if `T` is not registered.
    template <Event T>
    bool update() noexcept;

    /// @brief Drops every event of type `T`.
    /// @return False, doing nothing, if `T` is not registered.
    template <Event T>
    bool clear() noexcept;

  private:
    /// @brief Checks that `name` is not taken by another kind of type (WorldState::checkNameFree()). A member, so the
    /// templates, which only see a declaration of WorldState, can call it.
    /// @return Nothing when the name is free, or an ErrorKind::kNameCollision error.
    [[nodiscard]] Result<void> checkNameFree(std::string_view name) const;

    /// @brief Creates the queues of every registered event up to `id` that has none yet.
    void createStorages(EventId id);

    /// @return The queue of the event, or nullptr if the id is not registered in this world.
    [[nodiscard]] storage::EventStorage* findStorage(EventId id) noexcept;
    [[nodiscard]] const storage::EventStorage* findStorage(EventId id) const noexcept;

    const detail::WorldState* _state;  ///< The world's state, read for its components and resources.
    EventRegistry _registry;           ///< Events of this world, by id and name.
    /// @brief One queue per event, indexed by EventId; a deque keeps their addresses, so a range stays valid when
    /// other events are registered.
    std::deque<storage::EventStorage> _storages;
};
}  // namespace rtype::ecs

#include "Events.tpp"
