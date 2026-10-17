/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EventStorage
*/

#include "EventStorage.hpp"

#include <cassert>
#include <cstddef>
#include <memory_resource>
#include <span>
#include <utility>

#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/event/EventInfo.hpp"

namespace rtype::ecs::storage {
EventStorage::EventStorage(const EventInfo& info, std::pmr::memory_resource* memory)
    : _info{&info}, _previous{info.type, memory}, _current{info.type, memory} {}

const EventInfo& EventStorage::getInfo() const noexcept { return *_info; }

std::size_t EventStorage::getCount() const noexcept { return _previous.getSize() + _current.getSize(); }

EventSequence EventStorage::getFirst() const noexcept { return _previousStart; }

EventSequence EventStorage::getEnd() const noexcept { return _currentStart + _current.getSize(); }

std::span<const std::byte> EventStorage::get(EventSequence sequence) const noexcept {
    assert(sequence >= getFirst() && sequence < getEnd());
    if (sequence < _currentStart) {
        return _previous.at(static_cast<std::size_t>(sequence - _previousStart));
    }
    return _current.at(static_cast<std::size_t>(sequence - _currentStart));
}

void EventStorage::sendMove(std::span<std::byte> source) {
    assert(source.size() == _info->type.size);
    _current.emplaceBackMove(source);
}

void EventStorage::sendCopy(std::span<const std::byte> source) {
    assert(source.size() == _info->type.size);
    assert(_info->type.copyable);
    _current.emplaceBackCopy(source);
}

void EventStorage::update() noexcept {
    const EventSequence end = getEnd();
    _previous.clear();
    std::swap(_previous, _current);
    _previousStart = _currentStart;
    _currentStart = end;
}

void EventStorage::clear() noexcept {
    const EventSequence end = getEnd();
    _previous.clear();
    _current.clear();
    _previousStart = end;
    _currentStart = end;
}
}  // namespace rtype::ecs::storage
