/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Schedule
*/

#include <string>
#include <utility>

namespace rtype::ecs {
template <typename Function>
Schedule::SystemConfig Schedule::addSystem(std::string name, Function function) {
    return addSystem(System{*_world, std::move(name), function});
}
}  // namespace rtype::ecs
