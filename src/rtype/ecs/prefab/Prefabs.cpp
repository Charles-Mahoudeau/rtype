/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Prefabs
*/

#include "Prefabs.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <utility>

#include "core/hash/NameHash.hpp"
#include "rtype/ecs/exceptions/PrefabException.hpp"
#include "rtype/ecs/prefab/ComponentValues.hpp"
#include "rtype/ecs/prefab/Prefab.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::ecs {
RunMode Prefabs::getRunMode() const noexcept { return _mode; }

void Prefabs::setRunMode(RunMode mode) noexcept { _mode = mode; }

std::size_t Prefabs::getCount() const noexcept { return _prefabs.size(); }

PrefabId Prefabs::add(std::string name) {
    if (name.empty()) {
        throw exceptions::PrefabException{"A prefab needs a name"};
    }
    const std::uint64_t hash = core::hash::hashName(name);
    if (const auto found = _idsByHash.find(hash); found != _idsByHash.end()) {
        const std::string_view registered = _prefabs.at(found->second).getName();
        if (registered == name) {
            throw exceptions::PrefabException{std::format("Prefab '{}' is already registered", name)};
        }
        throw exceptions::PrefabException{
            std::format("Prefab '{}' has the same name hash as '{}': rename one of them", name, registered)};
    }
    const auto id = static_cast<PrefabId>(_prefabs.size());
    if (id == kInvalidPrefabId) {
        throw exceptions::PrefabException{std::format("Prefab '{}': every id is taken", name)};
    }
    _prefabs.emplace_back(id, std::move(name));
    _idsByHash.emplace(hash, id);
    return id;
}

const Prefab* Prefabs::findById(PrefabId id) const noexcept {
    return id < _prefabs.size() ? &_prefabs.at(id) : nullptr;
}

const Prefab* Prefabs::findByName(std::string_view name) const noexcept {
    const auto found = _idsByHash.find(core::hash::hashName(name));
    if (found == _idsByHash.end()) {
        return nullptr;
    }
    const Prefab& prefab = _prefabs.at(found->second);
    return prefab.getName() == name ? &prefab : nullptr;
}

Prefab& Prefabs::edit(PrefabId id) {
    if (id >= _prefabs.size()) {
        throw exceptions::PrefabException{std::format("No prefab has the id {}", id)};
    }
    return _prefabs.at(id);
}

void Prefabs::addChild(PrefabId parent, PrefabId child, ComponentValues overrides) {
    Prefab& parentPrefab = edit(parent);
    const Prefab& childPrefab = edit(child);
    if (contains(child, parent)) {
        throw exceptions::PrefabException{std::format("Prefab '{}' cannot have '{}' as a child: '{}' contains '{}'",
                                                      parentPrefab.getName(), childPrefab.getName(),
                                                      childPrefab.getName(), parentPrefab.getName())};
    }
    parentPrefab._children.push_back(PrefabChild{.prefab = child, .overrides = std::move(overrides)});
}

bool Prefabs::keeps(const TypeInfo& type) const noexcept {
    switch (type.presence) {
        case Presence::kServerOnly:
            return _mode != RunMode::kClient;
        case Presence::kClientOnly:
            return _mode != RunMode::kServer;
        case Presence::kBoth:
            break;
    }
    return true;
}

// NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the prefab tree, which addChild() keeps acyclic.
bool Prefabs::contains(PrefabId root, PrefabId target) const noexcept {
    if (root == target) {
        return true;
    }
    return std::ranges::any_of(_prefabs.at(root).getChildren(),
                               // NOLINTNEXTLINE(misc-no-recursion): the same recursion, through the predicate.
                               [&](const PrefabChild& child) { return contains(child.prefab, target); });
}
}  // namespace rtype::ecs
