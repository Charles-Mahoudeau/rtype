/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SystemParam
*/

#include "SystemParam.hpp"

#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
SystemParam<Commands&>::State SystemParam<Commands&>::init(World& /*world*/) noexcept { return {}; }

Commands& SystemParam<Commands&>::get(State& /*state*/, const SystemContext& context) noexcept {
    return *context.commands;
}

void SystemParam<Commands&>::addAccess(const State& /*state*/, SystemAccess& /*access*/) noexcept {}
}  // namespace rtype::ecs
