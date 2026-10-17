/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Query
*/

#include <array>
#include <cstddef>
#include <iterator>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/error/Result.hpp"

namespace rtype::ecs {
template <typename... Terms>
    requires ValidQuery<Terms...>
Query<Terms...>::Iteration::Iterator::Iterator(Iteration& iteration) noexcept : _iteration{&iteration} {}

template <typename... Terms>
    requires ValidQuery<Terms...>
typename Query<Terms...>::Item Query<Terms...>::Iteration::Iterator::operator*() const noexcept {
    return _iteration->makeItem(std::index_sequence_for<Terms...>{});
}

template <typename... Terms>
    requires ValidQuery<Terms...>
typename Query<Terms...>::Iteration::Iterator& Query<Terms...>::Iteration::Iterator::operator++() noexcept {
    _iteration->_cursor.advance();
    return *this;
}

template <typename... Terms>
    requires ValidQuery<Terms...>
void Query<Terms...>::Iteration::Iterator::operator++(int) noexcept {
    ++*this;
}

template <typename... Terms>
    requires ValidQuery<Terms...>
bool Query<Terms...>::Iteration::Iterator::operator==(std::default_sentinel_t /*sentinel*/) const noexcept {
    return _iteration->_cursor.isDone();
}

template <typename... Terms>
    requires ValidQuery<Terms...>
Query<Terms...>::Iteration::Iteration(const DynamicQuery& query, Tick lastRun, Tick thisRun) noexcept
    : _cursor{query.getTerms(), resolvePools(query), {}, lastRun, thisRun} {}

template <typename... Terms>
    requires ValidQuery<Terms...>
typename Query<Terms...>::Iteration::Iterator Query<Terms...>::Iteration::begin() noexcept {
    return Iterator{*this};
}

template <typename... Terms>
    requires ValidQuery<Terms...>
std::default_sentinel_t Query<Terms...>::Iteration::end() noexcept {
    return std::default_sentinel;
}

template <typename... Terms>
    requires ValidQuery<Terms...>
std::array<storage::SparseSetStorage*, sizeof...(Terms)> Query<Terms...>::Iteration::resolvePools(
    const DynamicQuery& query) noexcept {
    std::array<storage::SparseSetStorage*, sizeof...(Terms)> pools{};
    query.resolvePools(pools);
    return pools;
}

template <typename... Terms>
    requires ValidQuery<Terms...>
template <std::size_t... Indices>
typename Query<Terms...>::Item Query<Terms...>::Iteration::makeItem(
    std::index_sequence<Indices...> /*indices*/) const noexcept {
    return std::tuple_cat(std::tuple<Entity>{_cursor.getEntity()}, fetch<Indices>()...);
}

template <typename... Terms>
    requires ValidQuery<Terms...>
template <std::size_t Index>
auto Query<Terms...>::Iteration::fetch() const noexcept {
    using Term = std::tuple_element_t<Index, std::tuple<Terms...>>;
    if constexpr (TermTraits<Term>::kHasItem) {
        return std::tuple<typename TermTraits<Term>::Item>{TermTraits<Term>::fetch(
            *std::get<Index>(_cursor.getPools()), std::get<Index>(_cursor.getPositions()), _cursor.getThisRun())};
    } else {
        return std::tuple<>{};
    }
}

template <typename... Terms>
    requires ValidQuery<Terms...>
Query<Terms...>::Query(Components& components) : _components{&components}, _query{build(components)} {}

template <typename... Terms>
    requires ValidQuery<Terms...>
const DynamicQuery& Query<Terms...>::getDynamic() const noexcept {
    return _query;
}

template <typename... Terms>
    requires ValidQuery<Terms...>
std::span<const ComponentAccess> Query<Terms...>::getAccess() const noexcept {
    return _query.getAccess();
}

template <typename... Terms>
    requires ValidQuery<Terms...>
typename Query<Terms...>::Iteration Query<Terms...>::iter(Tick lastRun) const noexcept {
    return Iteration{_query, lastRun, _components->getChangeTick()};
}

template <typename... Terms>
    requires ValidQuery<Terms...>
DynamicQuery Query<Terms...>::build(Components& components) {
    std::vector<QueryTerm> terms{
        QueryTerm{.id = idOf<typename TermTraits<Terms>::Type>(components), .kind = TermTraits<Terms>::kKind}...};
    // Cannot fail: ValidQuery checked the terms at compile time, and every component is now registered.
    return std::move(DynamicQuery::create(components, std::move(terms))).value();
}

template <typename... Terms>
    requires ValidQuery<Terms...>
template <Component T>
ComponentId Query<Terms...>::idOf(Components& components) {
    if (const ComponentInfo* info = components.getRegistry().find<T>()) {
        return info->type.id;  // keeps the options it was registered with
    }
    return components.registerType<T>();
}
}  // namespace rtype::ecs
