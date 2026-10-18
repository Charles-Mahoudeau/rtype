/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ResourceParam
*/

#include <optional>

namespace rtype::ecs {
template <Resource T, ResourceAccess A, ResourceThread H>
ResourceParam<T, A, H>::ResourceParam(Resources& resources, Tick lastRun, Tick thisRun)
    : _value{fetch(resources)}, _ticks{*resources.getTicks<T>()}, _lastRun{lastRun}, _thisRun{thisRun} {}

template <Resource T, ResourceAccess A, ResourceThread H>
const T& ResourceParam<T, A, H>::get() const noexcept {
    if constexpr (A == ResourceAccess::kWrite) {
        return _value.get();
    } else {
        return *_value;
    }
}

template <Resource T, ResourceAccess A, ResourceThread H>
typename ResourceParam<T, A, H>::Pointer ResourceParam<T, A, H>::operator->() noexcept {
    if constexpr (A == ResourceAccess::kWrite) {
        return _value.operator->();  // marks the value changed
    } else {
        return _value;
    }
}

template <Resource T, ResourceAccess A, ResourceThread H>
typename ResourceParam<T, A, H>::Reference ResourceParam<T, A, H>::operator*() noexcept {
    return *_value;  // through a Mut for a written parameter: marks the value changed
}

template <Resource T, ResourceAccess A, ResourceThread H>
bool ResourceParam<T, A, H>::isAdded() const noexcept {
    return _ticks.isAdded(_lastRun, _thisRun);
}

template <Resource T, ResourceAccess A, ResourceThread H>
bool ResourceParam<T, A, H>::isChanged() const noexcept {
    return _ticks.isChanged(_lastRun, _thisRun);
}

template <Resource T, ResourceAccess A, ResourceThread H>
typename ResourceParam<T, A, H>::Value ResourceParam<T, A, H>::fetch(Resources& resources) {
    if constexpr (A == ResourceAccess::kWrite) {
        return resources.requireMut<T>();  // throws MissingResourceException naming it
    } else {
        return &resources.require<T>();
    }
}
}  // namespace rtype::ecs
