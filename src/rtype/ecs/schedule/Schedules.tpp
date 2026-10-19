/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Schedules
*/

#include <string>
#include <utility>

namespace rtype::ecs {
template <typename Function, typename... Configs>
Schedule::SystemConfig Schedules::addSystem(Phase phase, std::string name, Function function, Configs... configs) {
    return addSystem(phase, System{*_world, std::move(name), function, std::move(configs)...});
}
}  // namespace rtype::ecs
