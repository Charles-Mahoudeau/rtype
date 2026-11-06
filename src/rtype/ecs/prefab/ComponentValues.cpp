/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentValues
*/

#include "ComponentValues.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <format>
#include <iterator>
#include <span>
#include <utility>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/exceptions/PrefabException.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/storage/ByteColumn.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"

namespace rtype::ecs {
std::size_t ComponentValues::getCount() const noexcept { return _entries.size(); }

ComponentId ComponentValues::getId(std::size_t index) const noexcept {
    assert(index < _entries.size());
    return std::next(_entries.begin(), static_cast<std::ptrdiff_t>(index))->id;
}

std::span<const std::byte> ComponentValues::getValue(std::size_t index) const noexcept {
    assert(index < _entries.size());
    return std::next(_entries.begin(), static_cast<std::ptrdiff_t>(index))->column.at(0);
}

bool ComponentValues::has(ComponentId id) const noexcept { return find(id) != nullptr; }

std::span<const std::byte> ComponentValues::get(ComponentId id) const noexcept {
    const Entry* entry = find(id);
    return entry != nullptr ? entry->column.at(0) : std::span<const std::byte>{};
}

void ComponentValues::set(const Components& components, ComponentId id, std::span<const std::byte> value) {
    const ComponentInfo* info = components.getRegistry().findById(id);
    if (info == nullptr) {
        throw exceptions::PrefabException{std::format("Component id {} is not registered", id)};
    }
    if (info->type.name == TypeTraits<Parent>::kName || info->type.name == TypeTraits<Children>::kName) {
        throw exceptions::PrefabException{std::format(
            "Component '{}' cannot be a prefab value: a prefab's children set the hierarchy", info->type.name)};
    }
    if (value.size() != info->type.size) {
        throw exceptions::PrefabException{
            std::format("Component '{}' takes {} bytes, not {}", info->type.name, info->type.size, value.size())};
    }
    storage::ByteColumn column = makeColumn(*info);
    static_cast<void>(column.emplaceBackCopy(value));  // copied first: if it throws, the old value is kept
    store(id, std::move(column));
}

bool ComponentValues::remove(ComponentId id) noexcept {
    const auto found = std::ranges::find(_entries, id, &Entry::id);
    if (found == _entries.end()) {
        return false;
    }
    _entries.erase(found);
    return true;
}

const ComponentValues::Entry* ComponentValues::find(ComponentId id) const noexcept {
    const auto found = std::ranges::find(_entries, id, &Entry::id);
    return found != _entries.end() ? &*found : nullptr;
}

storage::ByteColumn ComponentValues::makeColumn(const ComponentInfo& info) {
    storage::ByteColumn column{info.type};
    column.reserve(1);  // one value: no room for the eight a first growth would make
    return column;
}

void ComponentValues::store(ComponentId id, storage::ByteColumn column) {
    const auto found = std::ranges::find(_entries, id, &Entry::id);
    if (found != _entries.end()) {
        found->column = std::move(column);  // the old value is destroyed
        return;
    }
    _entries.push_back(Entry{.id = id, .column = std::move(column)});
}
}  // namespace rtype::ecs
