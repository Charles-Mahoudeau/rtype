/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ByteColumn
*/

#include "ByteColumn.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <memory_resource>
#include <span>
#include <utility>

#include "rtype/ecs/component/ComponentInfo.hpp"

namespace rtype::ecs::storage {
namespace {
/// @brief Capacity of the first buffer, so small columns do not grow one value at a time.
constexpr std::size_t kMinimumCapacity = 8;
}  // namespace

void ByteColumn::BufferDelete::operator()(std::byte* data) const noexcept {
    resource->deallocate(data, bytes, alignment);
}

ByteColumn::ByteColumn(const ComponentInfo& info, std::pmr::memory_resource* resource)
    : _elementSize{info.size},
      _alignment{info.alignment},
      _hooks{info.hooks},
      _resource{resource},
      _data{nullptr, BufferDelete{.resource = resource, .bytes = 0, .alignment = info.alignment}} {}

ByteColumn::~ByteColumn() { clear(); }

ByteColumn::ByteColumn(ByteColumn&& other) noexcept
    : _elementSize{other._elementSize},
      _alignment{other._alignment},
      _hooks{other._hooks},
      _resource{other._resource},
      _data{std::move(other._data)},
      _size{std::exchange(other._size, 0)},
      _capacity{std::exchange(other._capacity, 0)} {}

ByteColumn& ByteColumn::operator=(ByteColumn&& other) noexcept {
    if (this != &other) {
        clear();
        _elementSize = other._elementSize;
        _alignment = other._alignment;
        _hooks = other._hooks;
        _resource = other._resource;
        _data = std::move(other._data);  // frees this column's old buffer
        _size = std::exchange(other._size, 0);
        _capacity = std::exchange(other._capacity, 0);
    }
    return *this;
}

std::size_t ByteColumn::getSize() const noexcept { return _size; }

std::size_t ByteColumn::getCapacity() const noexcept { return _capacity; }

std::size_t ByteColumn::getElementSize() const noexcept { return _elementSize; }

std::span<std::byte> ByteColumn::emplaceBack() {
    makeRoom();
    if (_elementSize == 0) {
        ++_size;
        return {};
    }
    const std::span<std::byte> destination = slot(_data.get(), _size);
    if (_hooks.construct != nullptr) {
        _hooks.construct(destination.data());
    } else {
        std::memset(destination.data(), 0, _elementSize);
    }
    ++_size;  // only once construction succeeded
    return destination;
}

std::span<std::byte> ByteColumn::emplaceBackCopy(std::span<const std::byte> source) {
    assert(source.size() == _elementSize);
    makeRoom();
    if (_elementSize == 0) {
        ++_size;
        return {};
    }
    const std::span<std::byte> destination = slot(_data.get(), _size);
    if (_hooks.copy != nullptr) {
        _hooks.copy(destination.data(), source.data());
    } else {
        std::memcpy(destination.data(), source.data(), _elementSize);
    }
    ++_size;
    return destination;
}

std::span<std::byte> ByteColumn::emplaceBackMove(std::span<std::byte> source) {
    assert(source.size() == _elementSize);
    makeRoom();
    if (_elementSize == 0) {
        ++_size;
        return {};
    }
    const std::span<std::byte> destination = slot(_data.get(), _size);
    if (_hooks.move != nullptr) {
        _hooks.move(destination.data(), source.data());
    } else {
        std::memcpy(destination.data(), source.data(), _elementSize);
    }
    ++_size;
    return destination;
}

void ByteColumn::swapRemove(std::size_t position) noexcept {
    assert(position < _size);
    const std::size_t last = _size - 1;
    if (_elementSize != 0) {
        std::byte* removed = slot(_data.get(), position).data();
        if (_hooks.destruct != nullptr) {
            _hooks.destruct(removed);
        }
        if (position != last) {
            std::byte* moved = slot(_data.get(), last).data();
            if (_hooks.move != nullptr) {
                _hooks.move(removed, moved);
                if (_hooks.destruct != nullptr) {
                    _hooks.destruct(moved);
                }
            } else {
                std::memcpy(removed, moved, _elementSize);
            }
        }
    }
    _size = last;
}

std::span<std::byte> ByteColumn::at(std::size_t position) noexcept {
    assert(position < _size);
    return slot(_data.get(), position);
}

std::span<const std::byte> ByteColumn::at(std::size_t position) const noexcept {
    assert(position < _size);
    return slot(_data.get(), position);
}

void ByteColumn::reserve(std::size_t capacity) {
    if (capacity > _capacity) {
        growTo(capacity);
    }
}

void ByteColumn::clear() noexcept {
    if (_hooks.destruct != nullptr) {
        for (std::size_t position = 0; position < _size; ++position) {
            _hooks.destruct(slot(_data.get(), position).data());
        }
    }
    _size = 0;
}

void ByteColumn::makeRoom() {
    if (_size == _capacity) {
        growTo(std::max(kMinimumCapacity, _capacity * 2));
    }
}

void ByteColumn::growTo(std::size_t capacity) {
    if (_elementSize == 0) {
        _capacity = capacity;  // a tag only counts its values: nothing to allocate
        return;
    }
    const std::size_t bytes = capacity * _elementSize;
    // Allocated before anything moves: if it throws, the column is unchanged.
    Buffer fresh{static_cast<std::byte*>(_resource->allocate(bytes, _alignment)),
                 BufferDelete{.resource = _resource, .bytes = bytes, .alignment = _alignment}};
    if (_size > 0) {
        if (_hooks.move == nullptr) {
            std::memcpy(fresh.get(), _data.get(), _size * _elementSize);
        } else {
            for (std::size_t position = 0; position < _size; ++position) {
                std::byte* old = slot(_data.get(), position).data();
                _hooks.move(slot(fresh.get(), position).data(), old);
                if (_hooks.destruct != nullptr) {
                    _hooks.destruct(old);
                }
            }
        }
    }
    _data = std::move(fresh);  // the old buffer goes back to the resource with its own size
    _capacity = capacity;
}

std::span<std::byte> ByteColumn::slot(std::byte* data, std::size_t position) const noexcept {
    return std::span<std::byte>{data, (position + 1) * _elementSize}.subspan(position * _elementSize);
}
}  // namespace rtype::ecs::storage
