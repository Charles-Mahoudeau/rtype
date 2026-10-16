/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EventRange
*/

#include "EventRange.hpp"

#include <cstddef>
#include <span>

#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/storage/EventStorage.hpp"

namespace rtype::ecs {
RawEventRange::Iterator::Iterator(const storage::EventStorage* queue, EventSequence sequence) noexcept
    : _queue{queue}, _sequence{sequence} {}

std::span<const std::byte> RawEventRange::Iterator::operator*() const noexcept { return _queue->get(_sequence); }

RawEventRange::Iterator& RawEventRange::Iterator::operator++() noexcept {
    ++_sequence;
    return *this;
}

bool RawEventRange::Iterator::operator==(const Iterator& other) const noexcept {
    return _queue == other._queue && _sequence == other._sequence;
}

RawEventRange::RawEventRange(const storage::EventStorage* queue, EventSequence first, EventSequence last) noexcept
    : _queue{queue}, _first{first}, _last{last} {}

std::size_t RawEventRange::size() const noexcept { return static_cast<std::size_t>(_last - _first); }

bool RawEventRange::empty() const noexcept { return _first == _last; }

RawEventRange::Iterator RawEventRange::begin() const noexcept { return Iterator{_queue, _first}; }

RawEventRange::Iterator RawEventRange::end() const noexcept { return Iterator{_queue, _last}; }
}  // namespace rtype::ecs
