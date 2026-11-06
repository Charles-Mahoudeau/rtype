/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Prefab
*/

#include "Prefab.hpp"

#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "rtype/ecs/prefab/ComponentValues.hpp"

namespace rtype::ecs {
Prefab::Prefab(PrefabId id, std::string name) : _id{id}, _name{std::move(name)} {}

PrefabId Prefab::getId() const noexcept { return _id; }

std::string_view Prefab::getName() const noexcept { return _name; }

const ComponentValues& Prefab::getValues() const noexcept { return _values; }

ComponentValues& Prefab::getValues() noexcept { return _values; }

std::span<const PrefabChild> Prefab::getChildren() const noexcept { return _children; }
}  // namespace rtype::ecs
