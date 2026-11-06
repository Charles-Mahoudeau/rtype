/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Actor
*/

#include "Actor.hpp"

#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "rtype/ecs/actor/ComponentValues.hpp"

namespace rtype::ecs {
Actor::Actor(ActorId id, std::string name) : _id{id}, _name{std::move(name)} {}

ActorId Actor::getId() const noexcept { return _id; }

std::string_view Actor::getName() const noexcept { return _name; }

const ComponentValues& Actor::getValues() const noexcept { return _values; }

ComponentValues& Actor::getValues() noexcept { return _values; }

std::span<const ActorChild> Actor::getChildren() const noexcept { return _children; }
}  // namespace rtype::ecs
