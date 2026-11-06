/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Actors
*/

#include "Actors.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <utility>

#include "core/hash/NameHash.hpp"
#include "rtype/ecs/actor/Actor.hpp"
#include "rtype/ecs/actor/ComponentValues.hpp"
#include "rtype/ecs/exceptions/ActorException.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::ecs {
RunMode Actors::getRunMode() const noexcept { return _mode; }

void Actors::setRunMode(RunMode mode) noexcept { _mode = mode; }

std::size_t Actors::getCount() const noexcept { return _actors.size(); }

ActorId Actors::add(std::string name) {
    if (name.empty()) {
        throw exceptions::ActorException{"An actor needs a name"};
    }
    const std::uint64_t hash = core::hash::hashName(name);
    if (const auto found = _idsByHash.find(hash); found != _idsByHash.end()) {
        const std::string_view registered = _actors.at(found->second).getName();
        if (registered == name) {
            throw exceptions::ActorException{std::format("Actor '{}' is already registered", name)};
        }
        throw exceptions::ActorException{
            std::format("Actor '{}' has the same name hash as '{}': rename one of them", name, registered)};
    }
    const auto id = static_cast<ActorId>(_actors.size());
    if (id == kInvalidActorId) {
        throw exceptions::ActorException{std::format("Actor '{}': every id is taken", name)};
    }
    _actors.emplace_back(id, std::move(name));
    _idsByHash.emplace(hash, id);
    return id;
}

const Actor* Actors::findById(ActorId id) const noexcept { return id < _actors.size() ? &_actors.at(id) : nullptr; }

const Actor* Actors::findByName(std::string_view name) const noexcept {
    const auto found = _idsByHash.find(core::hash::hashName(name));
    if (found == _idsByHash.end()) {
        return nullptr;
    }
    const Actor& actor = _actors.at(found->second);
    return actor.getName() == name ? &actor : nullptr;
}

Actor& Actors::edit(ActorId id) {
    if (id >= _actors.size()) {
        throw exceptions::ActorException{std::format("No actor has the id {}", id)};
    }
    return _actors.at(id);
}

void Actors::addChild(ActorId parent, ActorId child, ComponentValues overrides) {
    Actor& parentActor = edit(parent);
    const Actor& childActor = edit(child);
    if (contains(child, parent)) {
        throw exceptions::ActorException{std::format("Actor '{}' cannot have '{}' as a child: '{}' contains '{}'",
                                                     parentActor.getName(), childActor.getName(), childActor.getName(),
                                                     parentActor.getName())};
    }
    parentActor._children.push_back(ActorChild{.actor = child, .overrides = std::move(overrides)});
}

bool Actors::keeps(const TypeInfo& type) const noexcept {
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

// NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the actor tree, which addChild() keeps acyclic.
bool Actors::contains(ActorId root, ActorId target) const noexcept {
    if (root == target) {
        return true;
    }
    return std::ranges::any_of(_actors.at(root).getChildren(),
                               // NOLINTNEXTLINE(misc-no-recursion): the same recursion, through the predicate.
                               [&](const ActorChild& child) { return contains(child.actor, target); });
}
}  // namespace rtype::ecs
