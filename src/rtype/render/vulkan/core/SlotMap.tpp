/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SlotMap
*/

#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

namespace rtype::render::vulkan::core {

template <SlotKey Key, class Value>
Key SlotMap<Key, Value>::insert(Value value) {
    std::uint32_t index = 0;
    if (_freeSlots.empty()) {
        index = static_cast<std::uint32_t>(_slots.size());
        _slots.emplace_back();
    } else {
        index = _freeSlots.back();
        _freeSlots.pop_back();
    }
    Slot& slot = _slots.at(index);
    slot.value.emplace(std::move(value));
    ++_size;
    return Key{.index = index, .generation = slot.generation};
}

template <SlotKey Key, class Value>
Value* SlotMap<Key, Value>::find(Key key) {
    return slotOf(key) == nullptr ? nullptr : &*_slots.at(key.index).value;
}

template <SlotKey Key, class Value>
const Value* SlotMap<Key, Value>::find(Key key) const {
    const Slot* slot = slotOf(key);
    return slot == nullptr ? nullptr : &*slot->value;
}

template <SlotKey Key, class Value>
std::optional<Value> SlotMap<Key, Value>::erase(Key key) {
    if (slotOf(key) == nullptr) {
        return std::nullopt;
    }
    Slot& slot = _slots.at(key.index);
    std::optional<Value> value{std::move(slot.value)};
    slot.value.reset();
    // Skips 0 on wrap-around: it means "no value" in a key.
    slot.generation = slot.generation == std::numeric_limits<std::uint32_t>::max() ? 1 : slot.generation + 1;
    _freeSlots.push_back(key.index);
    --_size;
    return value;
}

template <SlotKey Key, class Value>
const SlotMap<Key, Value>::Slot* SlotMap<Key, Value>::slotOf(Key key) const {
    if (key.index >= _slots.size()) {
        return nullptr;
    }
    const Slot& slot = _slots.at(key.index);
    return slot.generation == key.generation && slot.value ? &slot : nullptr;
}

}  // namespace rtype::render::vulkan::core
