/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ActorConfig
*/

#include "ActorConfig.hpp"

#include <cstddef>
#include <format>
#include <span>
#include <string_view>

#include "rtype/ecs/actor/Actor.hpp"
#include "rtype/ecs/actor/Actors.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/exceptions/ActorException.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
ActorConfig::ActorConfig(World& world, ActorId id) noexcept : _world{&world}, _id{id} {}

ActorId ActorConfig::getId() const noexcept { return _id; }

ActorConfig& ActorConfig::with(ComponentId id, std::span<const std::byte> value) {
    _world->getActors().edit(_id).getValues().set(_world->getComponents(), id, value);
    return *this;
}

ActorId ActorConfig::findChild(std::string_view name) const {
    const Actor* child = _world->getActors().findByName(name);
    if (child == nullptr) {
        throw exceptions::ActorException{std::format("No actor is named '{}'", name)};
    }
    return child->getId();
}
}  // namespace rtype::ecs
