/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EntityAllocator
*/

#include "EntityAllocator.hpp"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>

#include "Entity.hpp"
#include "rtype/ecs/detail/Generation.hpp"
#include "rtype/ecs/exceptions/EntityExceptions.hpp"

namespace rtype::ecs {
std::size_t EntityAllocator::getAliveCount() const noexcept { return _aliveCount; }

Entity EntityAllocator::allocate() {
    const Entity entity = take(SlotState::kAlive);
    ++_aliveCount;
    return entity;
}

Entity EntityAllocator::reserve() {
    const Entity entity = take(SlotState::kReserved);
    _reserved.push_back(entity.getIndex());
    return entity;
}

void EntityAllocator::flushReserved() {
    for (const std::uint32_t index : _reserved) {
        Slot& slot = _slots.at(index);
        if (slot.state == SlotState::kReserved) {
            slot.state = SlotState::kAlive;
            ++_aliveCount;
        }
    }
    _reserved.clear();
}

bool EntityAllocator::release(Entity entity) noexcept {
    Slot* slot = find(entity);
    if (slot == nullptr) {
        return false;
    }
    if (slot->state == SlotState::kAlive) {
        --_aliveCount;
    } else if (slot->state != SlotState::kReserved) {
        return false;
    }
    if (const std::optional<std::uint32_t> next = detail::nextGeneration(slot->generation); next) {
        slot->generation = *next;
        slot->state = SlotState::kFree;
        _freeList.push_back(entity.getIndex());
    } else {
        slot->state = SlotState::kRetired;
    }
    return true;
}

bool EntityAllocator::isAlive(Entity entity) const noexcept {
    const Slot* slot = find(entity);
    return slot != nullptr && slot->state == SlotState::kAlive;
}

bool EntityAllocator::isReserved(Entity entity) const noexcept {
    const Slot* slot = find(entity);
    return slot != nullptr && slot->state == SlotState::kReserved;
}

Entity EntityAllocator::take(SlotState state) {
    if (!_freeList.empty()) {
        const std::uint32_t index = _freeList.back();
        _freeList.pop_back();
        Slot& slot = _slots.at(index);
        slot.state = state;
        return Entity{index, slot.generation};
    }
    if (_slots.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw exceptions::EntityLimitException{"No entity index left: every slot is in use or retired"};
    }
    const auto index = static_cast<std::uint32_t>(_slots.size());
    _slots.push_back(Slot{.generation = 1, .state = state});
    if (_freeList.capacity() < _slots.capacity()) {
        _freeList.reserve(_slots.capacity());
    }
    return Entity{index, 1};
}

EntityAllocator::Slot* EntityAllocator::find(Entity entity) noexcept {
    if (entity.isNull() || entity.getIndex() >= _slots.size()) {
        return nullptr;
    }
    const auto slot = std::next(_slots.begin(), static_cast<std::ptrdiff_t>(entity.getIndex()));
    return slot->generation == entity.getGeneration() ? &*slot : nullptr;
}

const EntityAllocator::Slot* EntityAllocator::find(Entity entity) const noexcept {
    if (entity.isNull() || entity.getIndex() >= _slots.size()) {
        return nullptr;
    }
    const auto slot = std::next(_slots.begin(), static_cast<std::ptrdiff_t>(entity.getIndex()));
    return slot->generation == entity.getGeneration() ? &*slot : nullptr;
}
}  // namespace rtype::ecs
