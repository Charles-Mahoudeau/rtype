/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EventCursor
*/

#pragma once

#include <cstdint>

namespace rtype::ecs {
/// @brief Number of an event in its queue: the events of one type are numbered from 0 in the order they are sent.
using EventSequence = std::uint64_t;

/// @brief Where one reader is in the queue of one event type: the number of the next event it has not read.
///
/// @details Each reader keeps its own cursor, so it sees each event exactly once even if it runs before the writer
/// in one update and after it in the next. A new cursor starts at 0: its first read returns every event still kept.
/// Use one cursor per event type.
/// @code
/// rtype::ecs::EventCursor cursor;
/// for (const Collision& collision : world.getEvents().read<Collision>(cursor)) {
///     // ...
/// }
/// @endcode
struct EventCursor {
    EventSequence next{0};  ///< Number of the next event to read.
};
}  // namespace rtype::ecs
