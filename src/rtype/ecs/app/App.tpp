/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** App
*/

#include <concepts>
#include <memory>
#include <string>
#include <utility>

namespace rtype::ecs {
template <std::derived_from<Plugin> P>
App& App::addPlugin(P plugin) {
    return addPlugin(std::make_unique<P>(std::move(plugin)));
}

template <std::derived_from<PluginGroup> G>
App& App::addPlugins(G group) {
    group.build(*this);
    return *this;
}

template <typename Function>
Schedule::SystemConfig App::addSystem(Phase phase, std::string name, Function function) {
    return _schedules.addSystem(phase, std::move(name), function);
}

template <Resource T>
App& App::insertResource(T value) {
    _world.getResources().insert(std::move(value));
    return *this;
}
}  // namespace rtype::ecs
