/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EventRange
*/

#pragma once

#include <cstddef>
#include <span>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/storage/EventStorage.hpp"

namespace rtype::ecs {
/// @brief The events a reader has not read yet, from one queue, as raw bytes: what Events::read() returns for an
/// event known only by id (the Luau bridge, the network).
///
/// @details It does not own the events: it is invalidated by the next update or clear of the queue. Events sent
/// after it was made are not part of it.
/// @code
/// for (const std::span<const std::byte> bytes : world.getEvents().read(collisionId, cursor)) {
///     // ...
/// }
/// @endcode
class RTYPE_ECS_API RawEventRange {
  public:
    /// @brief Walks the events of the range in the order they were sent.
    class RTYPE_ECS_API Iterator {
      public:
        /// @param queue The queue read; nullptr only for an empty range.
        /// @param sequence The number of the event this iterator is at.
        Iterator(const storage::EventStorage* queue, EventSequence sequence) noexcept;
        ~Iterator() = default;
        Iterator(const Iterator& other) noexcept = default;
        Iterator& operator=(const Iterator& other) noexcept = default;
        Iterator(Iterator&& other) noexcept = default;
        Iterator& operator=(Iterator&& other) noexcept = default;

        /// @return The bytes of the event, empty for an event type without data.
        [[nodiscard]] std::span<const std::byte> operator*() const noexcept;

        /// @brief Moves to the next event.
        Iterator& operator++() noexcept;

        /// @return Whether both iterators are at the same event.
        [[nodiscard]] bool operator==(const Iterator& other) const noexcept;

      private:
        const storage::EventStorage* _queue;  ///< The queue read.
        EventSequence _sequence;              ///< Number of the current event.
    };

    /// @param queue The queue read; nullptr for an empty range (an unknown event).
    /// @param first The number of the first event of the range.
    /// @param last One past the number of the last event of the range.
    RawEventRange(const storage::EventStorage* queue, EventSequence first, EventSequence last) noexcept;
    /// @brief An empty range.
    RawEventRange() noexcept = default;
    ~RawEventRange() = default;
    RawEventRange(const RawEventRange& other) noexcept = default;
    RawEventRange& operator=(const RawEventRange& other) noexcept = default;
    RawEventRange(RawEventRange&& other) noexcept = default;
    RawEventRange& operator=(RawEventRange&& other) noexcept = default;

    /// @return The number of events in the range.
    [[nodiscard]] std::size_t size() const noexcept;

    /// @return Whether the range has no event.
    [[nodiscard]] bool empty() const noexcept;

    /// @return An iterator at the oldest event of the range.
    [[nodiscard]] Iterator begin() const noexcept;

    /// @return An iterator one past the newest event of the range.
    [[nodiscard]] Iterator end() const noexcept;

  private:
    const storage::EventStorage* _queue{nullptr};  ///< The queue read; null for an empty range.
    EventSequence _first{0};                       ///< Number of the first event of the range.
    EventSequence _last{0};                        ///< One past the number of the last event of the range.
};

/// @brief The events of type `T` a reader has not read yet: what Events::read<T>() returns.
///
/// @details It does not own the events: it is invalidated by the next update or clear of the queue. Events sent
/// after it was made are not part of it. An event type without data can be counted (size()) but not dereferenced.
/// @code
/// for (const Collision& collision : world.getEvents().read<Collision>(cursor)) {
///     // ...
/// }
/// @endcode
template <Event T>
class EventRange {
  public:
    /// @brief Walks the events of the range in the order they were sent.
    class Iterator {
      public:
        /// @param raw The same position, over raw bytes.
        explicit Iterator(RawEventRange::Iterator raw) noexcept;
        ~Iterator() = default;
        Iterator(const Iterator& other) noexcept = default;
        Iterator& operator=(const Iterator& other) noexcept = default;
        Iterator(Iterator&& other) noexcept = default;
        Iterator& operator=(Iterator&& other) noexcept = default;

        /// @return The event.
        [[nodiscard]] const T& operator*() const noexcept;

        /// @return The event.
        [[nodiscard]] const T* operator->() const noexcept;

        /// @brief Moves to the next event.
        Iterator& operator++() noexcept;

        /// @return Whether both iterators are at the same event.
        [[nodiscard]] bool operator==(const Iterator& other) const noexcept;

      private:
        RawEventRange::Iterator _raw;  ///< The same position, over raw bytes.
    };

    /// @param raw The events of the range, as raw bytes of `T`.
    explicit EventRange(RawEventRange raw) noexcept;
    /// @brief An empty range.
    EventRange() noexcept = default;
    ~EventRange() = default;
    EventRange(const EventRange& other) noexcept = default;
    EventRange& operator=(const EventRange& other) noexcept = default;
    EventRange(EventRange&& other) noexcept = default;
    EventRange& operator=(EventRange&& other) noexcept = default;

    /// @return The number of events in the range.
    [[nodiscard]] std::size_t size() const noexcept;

    /// @return Whether the range has no event.
    [[nodiscard]] bool empty() const noexcept;

    /// @return An iterator at the oldest event of the range.
    [[nodiscard]] Iterator begin() const noexcept;

    /// @return An iterator one past the newest event of the range.
    [[nodiscard]] Iterator end() const noexcept;

  private:
    RawEventRange _raw;  ///< The events, as raw bytes.
};
}  // namespace rtype::ecs

#include "EventRange.tpp"
