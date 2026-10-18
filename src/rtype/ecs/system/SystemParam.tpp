/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SystemParam
*/

#include "rtype/ecs/event/EventRegistry.hpp"
#include "rtype/ecs/query/QueryTerm.hpp"
#include "rtype/ecs/resource/ResourceRegistry.hpp"

namespace rtype::ecs {
template <typename... Terms>
    requires ValidQuery<Terms...>
typename SystemParam<Query<Terms...>>::State SystemParam<Query<Terms...>>::init(World& world) {
    return State{world.getComponents()};
}

template <typename... Terms>
    requires ValidQuery<Terms...>
Query<Terms...> SystemParam<Query<Terms...>>::get(State& state, const SystemContext& context) noexcept {
    return Query<Terms...>{state, context.lastRun};
}

template <typename... Terms>
    requires ValidQuery<Terms...>
void SystemParam<Query<Terms...>>::addAccess(const State& state, SystemAccess& access) {
    for (const ComponentAccess& component : state.getAccess()) {
        access.addComponent(component.id, component.write);
    }
}

template <Resource T, ResourceAccess A, ResourceThread H>
typename SystemParam<ResourceParam<T, A, H>>::State SystemParam<ResourceParam<T, A, H>>::init(World& world) {
    if (const ResourceInfo* info = world.getResources().getRegistry().find<T>()) {
        return info->type.id;  // keeps the options it was registered with
    }
    return world.getResources().registerType<T>(ResourceOptions{.nonSend = H == ResourceThread::kMain});
}

template <Resource T, ResourceAccess A, ResourceThread H>
ResourceParam<T, A, H> SystemParam<ResourceParam<T, A, H>>::get(State& /*state*/, const SystemContext& context) {
    return ResourceParam<T, A, H>{context.world->getResources(), context.lastRun, context.thisRun};
}

template <Resource T, ResourceAccess A, ResourceThread H>
void SystemParam<ResourceParam<T, A, H>>::addAccess(const State& state, SystemAccess& access) {
    access.addResource(state, A == ResourceAccess::kWrite);
    if constexpr (H == ResourceThread::kMain) {
        access.requireMainThread();
    }
}

template <Event T>
typename SystemParam<EventReader<T>>::State SystemParam<EventReader<T>>::init(World& world) {
    const EventInfo* info = world.getEvents().getRegistry().find<T>();
    return State{.id = info != nullptr ? info->type.id : world.getEvents().registerType<T>(), .cursor = {}};
}

template <Event T>
EventReader<T> SystemParam<EventReader<T>>::get(State& state, const SystemContext& context) noexcept {
    return EventReader<T>{context.world->getEvents(), state.cursor};
}

template <Event T>
void SystemParam<EventReader<T>>::addAccess(const State& state, SystemAccess& access) {
    access.addEvent(state.id, false);
}

template <Event T>
typename SystemParam<EventWriter<T>>::State SystemParam<EventWriter<T>>::init(World& world) {
    const EventInfo* info = world.getEvents().getRegistry().find<T>();
    return info != nullptr ? info->type.id : world.getEvents().registerType<T>();
}

template <Event T>
EventWriter<T> SystemParam<EventWriter<T>>::get(State& /*state*/, const SystemContext& context) noexcept {
    return EventWriter<T>{context.world->getEvents()};
}

template <Event T>
void SystemParam<EventWriter<T>>::addAccess(const State& state, SystemAccess& access) {
    access.addEvent(state, true);
}
}  // namespace rtype::ecs
