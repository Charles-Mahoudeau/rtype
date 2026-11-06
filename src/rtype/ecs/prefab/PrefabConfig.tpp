/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PrefabConfig
*/

#pragma once

#ifndef RTYPE_PREFAB_CONFIG_INCLUDING_TPP
#error "PrefabConfig.tpp is an implementation detail of PrefabConfig.hpp: include PrefabConfig.hpp instead"
#endif

#include <string_view>
#include <utility>

#include "rtype/ecs/prefab/ComponentValues.hpp"
#include "rtype/ecs/prefab/Prefab.hpp"
#include "rtype/ecs/prefab/Prefabs.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
template <Component T>
PrefabConfig& PrefabConfig::with(T value) {
    _world->getPrefabs().edit(_id).getValues().set(_world->getComponents(), std::move(value));
    return *this;
}

template <Component... Overrides>
PrefabConfig& PrefabConfig::withChild(std::string_view child, Overrides... overrides) {
    return withChild(findChild(child), std::move(overrides)...);
}

template <Component... Overrides>
PrefabConfig& PrefabConfig::withChild(PrefabId child, Overrides... overrides) {
    ComponentValues values;
    (values.set(_world->getComponents(), std::move(overrides)), ...);
    _world->getPrefabs().addChild(_id, child, std::move(values));
    return *this;
}
}  // namespace rtype::ecs
