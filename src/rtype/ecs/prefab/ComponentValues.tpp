/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentValues
*/

#pragma once

#ifndef RTYPE_COMPONENT_VALUES_INCLUDING_TPP
#error "ComponentValues.tpp is an implementation detail of ComponentValues.hpp: include ComponentValues.hpp instead"
#endif

#include <concepts>
#include <cstddef>
#include <span>
#include <type_traits>
#include <utility>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/storage/ByteColumn.hpp"
#include "rtype/ecs/storage/ViewAs.hpp"

namespace rtype::ecs {
template <Component T>
const T* ComponentValues::get(const Components& components) const noexcept {
    static_assert(!std::is_empty_v<T>, "a tag has no value: test it with has()");
    const ComponentInfo* info = components.getRegistry().find<T>();
    if (info == nullptr) {
        return nullptr;
    }
    const std::span<const std::byte> bytes = get(info->type.id);
    return bytes.empty() ? nullptr : storage::viewAs<T>(bytes);
}

template <Component T>
void ComponentValues::set(Components& components, T value) {
    static_assert(!std::same_as<T, Parent> && !std::same_as<T, Children>,
                  "Parent and Children cannot be prefab values: a prefab's children set the hierarchy");
    const ComponentInfo* info = components.getRegistry().find<T>();
    if (info == nullptr) {
        info = components.getRegistry().findById(components.registerType<T>());
    }
    storage::ByteColumn column = makeColumn(*info);
    static_cast<void>(column.emplaceBackMove(std::as_writable_bytes(std::span{&value, 1}).first(info->type.size)));
    store(info->type.id, std::move(column));
}  // `value`, now moved-from, is destroyed when this function returns
}  // namespace rtype::ecs
