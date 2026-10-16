/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SparseSetStorage
*/

#include "SparseSetStorage.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory_resource>
#include <optional>
#include <span>

#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::ecs::storage {
namespace {
/// @brief Bound on the number of values: dense positions are stored in 32 bits, and the largest value means "no
/// entry" in the sparse array. Only read by an assert, so unused in release builds.
[[maybe_unused]] constexpr std::uint32_t kMaxPositions = std::numeric_limits<std::uint32_t>::max();
}  // namespace

SparseSetStorage::SparseSetStorage(const ComponentInfo& info, std::pmr::memory_resource* resource)
    : _info{&info}, _data{info.type, resource} {}

const ComponentInfo& SparseSetStorage::getInfo() const noexcept { return *_info; }

std::size_t SparseSetStorage::getSize() const noexcept { return _dense.size(); }

std::span<const Entity> SparseSetStorage::getEntities() const noexcept { return _dense; }

std::span<Tick> SparseSetStorage::getAddedTicks() noexcept { return _added; }

std::span<const Tick> SparseSetStorage::getAddedTicks() const noexcept { return _added; }

std::span<Tick> SparseSetStorage::getChangedTicks() noexcept { return _changed; }

std::span<const Tick> SparseSetStorage::getChangedTicks() const noexcept { return _changed; }

bool SparseSetStorage::contains(Entity entity) const noexcept { return static_cast<bool>(find(entity)); }

std::optional<std::uint32_t> SparseSetStorage::find(Entity entity) const noexcept {
    const std::optional<std::uint32_t> position = _sparse.get(entity.getIndex());
    if (position && *std::next(_dense.begin(), *position) == entity) {
        return position;
    }
    return std::nullopt;
}

std::span<std::byte> SparseSetStorage::get(Entity entity) noexcept {
    if (const std::optional<std::uint32_t> position = find(entity)) {
        return _data.at(*position);
    }
    return {};
}

std::span<const std::byte> SparseSetStorage::get(Entity entity) const noexcept {
    if (const std::optional<std::uint32_t> position = find(entity)) {
        return _data.at(*position);
    }
    return {};
}

std::span<std::byte> SparseSetStorage::at(std::size_t position) noexcept { return _data.at(position); }

std::span<const std::byte> SparseSetStorage::at(std::size_t position) const noexcept { return _data.at(position); }

std::span<std::byte> SparseSetStorage::emplace(Entity entity, Tick tick) {
    const std::span<std::byte> value = _data.emplaceBack();
    track(entity, tick);
    return value;
}

std::span<std::byte> SparseSetStorage::emplaceCopy(Entity entity, std::span<const std::byte> source, Tick tick) {
    const std::span<std::byte> value = _data.emplaceBackCopy(source);
    track(entity, tick);
    return value;
}

std::span<std::byte> SparseSetStorage::emplaceMove(Entity entity, std::span<std::byte> source, Tick tick) {
    const std::span<std::byte> value = _data.emplaceBackMove(source);
    track(entity, tick);
    return value;
}

bool SparseSetStorage::replaceCopy(Entity entity, std::span<const std::byte> source, Tick tick) {
    const std::optional<std::uint32_t> position = find(entity);
    if (!position) {
        return false;
    }
    _data.emplaceBackCopy(source);  // the new value first: if it throws, nothing changed
    _data.swapRemove(*position);    // destroys the old value and moves the new one into its slot
    *std::next(_changed.begin(), *position) = tick;
    return true;
}

bool SparseSetStorage::replaceMove(Entity entity, std::span<std::byte> source, Tick tick) {
    const std::optional<std::uint32_t> position = find(entity);
    if (!position) {
        return false;
    }
    _data.emplaceBackMove(source);
    _data.swapRemove(*position);
    *std::next(_changed.begin(), *position) = tick;
    return true;
}

bool SparseSetStorage::remove(Entity entity) noexcept {
    const std::optional<std::uint32_t> position = find(entity);
    if (!position) {
        return false;
    }
    _data.swapRemove(*position);
    if (*position != _dense.size() - 1) {
        const Entity moved = _dense.back();
        *std::next(_dense.begin(), *position) = moved;
        *std::next(_added.begin(), *position) = _added.back();
        *std::next(_changed.begin(), *position) = _changed.back();
        _sparse.set(moved.getIndex(), *position);  // its page exists already: nothing to allocate
    }
    _dense.pop_back();
    _added.pop_back();
    _changed.pop_back();
    _sparse.erase(entity.getIndex());
    return true;
}

void SparseSetStorage::clear() noexcept {
    for (const Entity entity : _dense) {
        _sparse.erase(entity.getIndex());
    }
    _dense.clear();
    _data.clear();
    _added.clear();
    _changed.clear();
}

void SparseSetStorage::reserve(std::size_t capacity) {
    _dense.reserve(capacity);
    _data.reserve(capacity);
    _added.reserve(capacity);
    _changed.reserve(capacity);
}

void SparseSetStorage::track(Entity entity, Tick tick) {
    assert(!entity.isNull());
    assert(!_sparse.get(entity.getIndex()));
    assert(_dense.size() < kMaxPositions);
    const auto position = static_cast<std::uint32_t>(_dense.size());
    try {
        _dense.push_back(entity);
        _added.push_back(tick);
        _changed.push_back(tick);
        _sparse.set(entity.getIndex(), position);
    } catch (...) {
        _dense.resize(position);
        _added.resize(position);
        _changed.resize(position);
        _data.swapRemove(position);  // the last value: destroyed, nothing moves
        throw;
    }
}
}  // namespace rtype::ecs::storage
