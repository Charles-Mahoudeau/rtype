/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EventStorage
*/

#pragma once

#include <cstddef>
#include <memory_resource>
#include <span>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/event/EventInfo.hpp"
#include "rtype/ecs/storage/ByteColumn.hpp"

namespace rtype::ecs::storage {
/// @brief The double-buffered queue of one event type: the events sent since the last update, and those of the
/// update before, stored as raw bytes.
///
/// @details Sending appends to the current buffer. update() drops the previous buffer and makes the current one
/// previous, so an event is kept for the update it was sent in and the next one, then dropped. Events are numbered in
/// the order they are sent (EventSequence): the previous buffer holds [getFirst(), the first current event), the
/// current one the rest up to getEnd(). Buffers are cleared rather than freed, so a queue in steady state does not
/// allocate. The storage knows the event only through its EventInfo; the columns handle alignment and the lifecycle
/// hooks.
///
/// Spans returned by the storage are invalidated by sending (a buffer may grow) and by update() and clear().
/// @code
/// const rtype::ecs::EventInfo info = rtype::ecs::EventTraits<Collision>::makeInfo();
/// rtype::ecs::storage::EventStorage queue{info};
/// Collision collision{.a = bullet, .b = enemy};
/// queue.sendMove(std::as_writable_bytes(std::span{&collision, 1}));
/// queue.update();  // still readable until the next update
/// @endcode
class RTYPE_ECS_API EventStorage {
  public:
    /// @brief An empty queue for the described event.
    /// @param info Must outlive the storage: the registry keeps every EventInfo at a stable address.
    /// @param memory Where the buffers come from; it must outlive the storage.
    explicit EventStorage(const EventInfo& info, std::pmr::memory_resource* memory = std::pmr::get_default_resource());
    /// @brief Destroys every event kept.
    ~EventStorage() = default;
    EventStorage(const EventStorage& other) = delete;
    EventStorage& operator=(const EventStorage& other) = delete;
    EventStorage(EventStorage&& other) noexcept = default;
    EventStorage& operator=(EventStorage&& other) noexcept = default;

    /// @return The description of the stored event.
    [[nodiscard]] const EventInfo& getInfo() const noexcept;

    /// @return The number of events kept: those of the previous and of the current update.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @return The number of the oldest event kept; equal to getEnd() when there is none.
    [[nodiscard]] EventSequence getFirst() const noexcept;

    /// @return The number the next event sent will get: one past the newest event kept.
    [[nodiscard]] EventSequence getEnd() const noexcept;

    /// @return The bytes of the event with this number, empty for an event type without data.
    /// @note Not checked in release builds: `sequence` must be in [getFirst(), getEnd()).
    [[nodiscard]] std::span<const std::byte> get(EventSequence sequence) const noexcept;

    /// @brief Appends an event moved from `source` to the current buffer.
    /// @details `source` stays a valid, moved-from value: its owner must still destroy it. If the buffer cannot grow,
    /// the queue is unchanged.
    /// @param source The bytes of a live value of this event's type, exactly its size.
    void sendMove(std::span<std::byte> source);

    /// @brief Appends a copy of an event to the current buffer. If copying throws, the queue is unchanged.
    /// @param source The bytes of a live value of this event's type, exactly its size. The event must be copyable
    /// (TypeInfo::copyable).
    void sendCopy(std::span<const std::byte> source);

    /// @brief Drops the events of the previous update and makes the current ones previous.
    void update() noexcept;

    /// @brief Drops every event kept. Numbering goes on from getEnd(), so cursors stay valid.
    void clear() noexcept;

  private:
    const EventInfo* _info;           ///< Description of the event, owned by the registry.
    ByteColumn _previous;             ///< Events of the previous update.
    ByteColumn _current;              ///< Events sent since the last update.
    EventSequence _previousStart{0};  ///< Number of the first event in `_previous`.
    EventSequence _currentStart{0};   ///< Number of the first event in `_current`.
};
}  // namespace rtype::ecs::storage
