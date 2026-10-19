/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Schedule
*/

#include <string>
#include <utility>

namespace rtype::ecs {
template <typename Function, typename... Configs>
Schedule::SystemConfig Schedule::addSystem(std::string name, Function function, Configs... configs) {
    return addSystem(System{*_world, std::move(name), std::move(function), std::move(configs)...});
}
}  // namespace rtype::ecs
