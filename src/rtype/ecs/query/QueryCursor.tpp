/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryCursor
*/

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <utility>

#include "rtype/ecs/query/QueryMatcher.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"

namespace rtype::ecs::detail {
template <typename Pools, typename Positions>
QueryCursor<Pools, Positions>::QueryCursor(std::span<const QueryTerm> terms, Pools pools, Positions positions,
                                           Tick lastRun, Tick thisRun) noexcept
    : _terms{terms},
      _pools{std::move(pools)},
      _positions{std::move(positions)},
      _driver{findDriver(_terms, _pools)},
      _lastRun{lastRun},
      _thisRun{thisRun} {
    skipToMatch();
}

template <typename Pools, typename Positions>
const Pools& QueryCursor<Pools, Positions>::getPools() const noexcept {
    return _pools;
}

template <typename Pools, typename Positions>
const Positions& QueryCursor<Pools, Positions>::getPositions() const noexcept {
    return _positions;
}

template <typename Pools, typename Positions>
Tick QueryCursor<Pools, Positions>::getThisRun() const noexcept {
    return _thisRun;
}

template <typename Pools, typename Positions>
Entity QueryCursor<Pools, Positions>::getEntity() const noexcept {
    return _entity;
}

template <typename Pools, typename Positions>
bool QueryCursor<Pools, Positions>::isDone() const noexcept {
    return _index >= (*std::next(_pools.begin(), static_cast<std::ptrdiff_t>(_driver)))->getSize();
}

template <typename Pools, typename Positions>
void QueryCursor<Pools, Positions>::advance() noexcept {
    ++_index;
    skipToMatch();
}

template <typename Pools, typename Positions>
void QueryCursor<Pools, Positions>::skipToMatch() noexcept {
    const std::span<const Entity> entities =
        (*std::next(_pools.begin(), static_cast<std::ptrdiff_t>(_driver)))->getEntities();
    for (; _index < entities.size(); ++_index) {
        const Entity entity = *std::next(entities.begin(), static_cast<std::ptrdiff_t>(_index));
        if (matchEntity(_terms, entity, _driver, static_cast<std::uint32_t>(_index), _pools, _positions, _lastRun,
                        _thisRun)) {
            _entity = entity;
            return;
        }
    }
}
}  // namespace rtype::ecs::detail
