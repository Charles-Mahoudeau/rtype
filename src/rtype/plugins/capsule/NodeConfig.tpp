/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NodeConfig
*/

#pragma once

#ifndef RTYPE_NODE_CONFIG_INCLUDING_TPP
#error "NodeConfig.tpp is an implementation detail of NodeConfig.hpp: include NodeConfig.hpp instead"
#endif

#include <utility>

#include "plugins/capsule/ComponentValues.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::plugins::capsule {
template <ecs::Component T>
NodeConfig& NodeConfig::with(T value) {
    editNode().values.set(_world->getComponents(), std::move(value));
    return *this;
}
}  // namespace rtype::plugins::capsule
