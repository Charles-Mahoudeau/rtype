/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryIteration
*/

#include "QueryIteration.hpp"

#include <cstddef>
#include <iterator>
#include <span>
#include <vector>

#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/DynamicQuery.hpp"
#include "rtype/ecs/query/QueryMatch.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"

namespace rtype::ecs {
namespace {
/// @return The pool of every term of `query`, in term order.
[[nodiscard]] std::vector<storage::SparseSetStorage*> resolvePools(const DynamicQuery& query) {
    std::vector<storage::SparseSetStorage*> pools(query.getTerms().size(), nullptr);
    query.resolvePools(pools);
    return pools;
}
}  // namespace

QueryIteration::Iterator::Iterator(QueryIteration& iteration, std::size_t index) noexcept
    : _iteration{&iteration}, _index{index} {
    skipToMatch();
}

QueryMatch& QueryIteration::Iterator::operator*() const noexcept { return _iteration->_match; }

QueryIteration::Iterator& QueryIteration::Iterator::operator++() noexcept {
    ++_index;
    skipToMatch();
    return *this;
}

bool QueryIteration::Iterator::operator==(std::default_sentinel_t /*sentinel*/) const noexcept {
    return _index >=
           (*std::next(_iteration->_pools.begin(), static_cast<std::ptrdiff_t>(_iteration->_driver)))->getSize();
}

void QueryIteration::Iterator::skipToMatch() noexcept {
    QueryIteration& iteration = *_iteration;
    const std::span<const Entity> entities =
        (*std::next(iteration._pools.begin(), static_cast<std::ptrdiff_t>(iteration._driver)))->getEntities();
    for (; _index < entities.size(); ++_index) {
        const Entity entity = *std::next(entities.begin(), static_cast<std::ptrdiff_t>(_index));
        if (iteration._query->match(entity, iteration._pools, iteration._positions, iteration._lastRun,
                                    iteration._thisRun)) {
            iteration._match.setEntity(entity);
            return;
        }
    }
}

QueryIteration::QueryIteration(const DynamicQuery& query, Tick lastRun, Tick thisRun)
    : _query{&query},
      _pools{resolvePools(query)},
      _positions(query.getTerms().size(), DynamicQuery::kNoPosition),
      _driver{query.findDriver(_pools)},
      _lastRun{lastRun},
      _thisRun{thisRun},
      _match{query, _pools, _positions, thisRun} {}

QueryIteration::Iterator QueryIteration::begin() noexcept { return Iterator{*this, 0}; }

std::default_sentinel_t QueryIteration::end() noexcept { return std::default_sentinel; }
}  // namespace rtype::ecs
