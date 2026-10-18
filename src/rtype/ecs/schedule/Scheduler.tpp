/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Scheduler
*/

#include <string>
#include <utility>

namespace rtype::ecs {
template <typename Function>
Scheduler::SystemConfig Scheduler::addSystem(Phase phase, std::string name, Function function) {
    return addSystem(phase, System{*_world, std::move(name), function});
}
}  // namespace rtype::ecs
