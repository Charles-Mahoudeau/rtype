/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ActorConfig
*/

#pragma once

#ifndef RTYPE_ACTOR_CONFIG_INCLUDING_TPP
#error "ActorConfig.tpp is an implementation detail of ActorConfig.hpp: include ActorConfig.hpp instead"
#endif

#include <string_view>
#include <utility>

#include "rtype/ecs/actor/Actor.hpp"
#include "rtype/ecs/actor/Actors.hpp"
#include "rtype/ecs/actor/ComponentValues.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
template <Component T>
ActorConfig& ActorConfig::with(T value) {
    _world->getActors().edit(_id).getValues().set(_world->getComponents(), std::move(value));
    return *this;
}

template <Component... Overrides>
ActorConfig& ActorConfig::withChild(std::string_view child, Overrides... overrides) {
    return withChild(findChild(child), std::move(overrides)...);
}

template <Component... Overrides>
ActorConfig& ActorConfig::withChild(ActorId child, Overrides... overrides) {
    ComponentValues values;
    (values.set(_world->getComponents(), std::move(overrides)), ...);
    _world->getActors().addChild(_id, child, std::move(values));
    return *this;
}
}  // namespace rtype::ecs
