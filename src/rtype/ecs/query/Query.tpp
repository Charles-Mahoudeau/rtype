/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Query
*/

#include <iterator>

namespace rtype::ecs {
template <typename... Terms>
    requires ValidQuery<Terms...>
Query<Terms...>::Query(const State& state, Tick lastRun) noexcept : _state{&state}, _lastRun{lastRun} {}

template <typename... Terms>
    requires ValidQuery<Terms...>
Tick Query<Terms...>::getLastRun() const noexcept {
    return _lastRun;
}

template <typename... Terms>
    requires ValidQuery<Terms...>
auto Query<Terms...>::begin() noexcept -> Iterator {
    _pass.emplace(_state->iter(_lastRun));
    return _pass->begin();
}

template <typename... Terms>
    requires ValidQuery<Terms...>
std::default_sentinel_t Query<Terms...>::end() noexcept {
    return std::default_sentinel;
}
}  // namespace rtype::ecs
