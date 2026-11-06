/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PrefabConfig
*/

#include "PrefabConfig.hpp"

#include <cstddef>
#include <format>
#include <span>
#include <string_view>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/exceptions/PrefabException.hpp"
#include "rtype/ecs/prefab/Prefab.hpp"
#include "rtype/ecs/prefab/Prefabs.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
PrefabConfig::PrefabConfig(World& world, PrefabId id) noexcept : _world{&world}, _id{id} {}

PrefabId PrefabConfig::getId() const noexcept { return _id; }

PrefabConfig& PrefabConfig::with(ComponentId id, std::span<const std::byte> value) {
    _world->getPrefabs().edit(_id).getValues().set(_world->getComponents(), id, value);
    return *this;
}

PrefabId PrefabConfig::findChild(std::string_view name) const {
    const Prefab* child = _world->getPrefabs().findByName(name);
    if (child == nullptr) {
        throw exceptions::PrefabException{std::format("No prefab is named '{}'", name)};
    }
    return child->getId();
}
}  // namespace rtype::ecs
