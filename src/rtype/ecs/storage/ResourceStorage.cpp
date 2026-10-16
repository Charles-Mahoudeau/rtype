/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ResourceStorage
*/

#include "ResourceStorage.hpp"

#include <cassert>
#include <cstddef>
#include <memory_resource>
#include <span>

#include "rtype/ecs/change/ComponentTicks.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"

namespace rtype::ecs::storage {
ResourceStorage::ResourceStorage(const ResourceInfo& info, std::pmr::memory_resource* memory)
    : _info{&info}, _value{info.type, memory} {}

const ResourceInfo& ResourceStorage::getInfo() const noexcept { return *_info; }

ComponentTicks& ResourceStorage::getTicks() noexcept { return _ticks; }

const ComponentTicks& ResourceStorage::getTicks() const noexcept { return _ticks; }

bool ResourceStorage::hasValue() const noexcept { return _value.getSize() != 0; }

std::span<std::byte> ResourceStorage::get() noexcept { return hasValue() ? _value.at(0) : std::span<std::byte>{}; }

std::span<const std::byte> ResourceStorage::get() const noexcept {
    return hasValue() ? _value.at(0) : std::span<const std::byte>{};
}

std::span<std::byte> ResourceStorage::insertMove(std::span<std::byte> source, Tick tick) {
    assert(source.size() == _info->type.size);
    _value.reserve(1);
    const bool replaced = hasValue();
    _value.clear();
    const std::span<std::byte> stored = _value.emplaceBackMove(source);
    stampInsertion(replaced, tick);
    return stored;
}

std::span<std::byte> ResourceStorage::insertCopy(std::span<const std::byte> source, Tick tick) {
    assert(source.size() == _info->type.size);
    assert(_info->type.copyable);
    if (!hasValue()) {
        _value.reserve(1);
        const std::span<std::byte> stored = _value.emplaceBackCopy(source);
        stampInsertion(false, tick);
        return stored;
    }
    _value.reserve(2);
    _value.emplaceBackCopy(source);
    _value.swapRemove(0);
    stampInsertion(true, tick);
    return _value.at(0);
}

bool ResourceStorage::remove() noexcept {
    if (!hasValue()) {
        return false;
    }
    _value.clear();
    return true;
}

void ResourceStorage::clampTicks(Tick thisRun) noexcept {
    clampTick(_ticks.added, thisRun);
    clampTick(_ticks.changed, thisRun);
}

void ResourceStorage::stampInsertion(bool replaced, Tick tick) noexcept {
    if (!replaced) {
        _ticks.added = tick;
    }
    _ticks.changed = tick;
}
}  // namespace rtype::ecs::storage
