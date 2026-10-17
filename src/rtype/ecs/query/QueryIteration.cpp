/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryIteration
*/

#include "QueryIteration.hpp"

#include <cstdint>
#include <iterator>
#include <vector>

#include "rtype/ecs/change/Tick.hpp"
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

QueryIteration::Iterator::Iterator(QueryIteration& iteration) noexcept : _iteration{&iteration} {}

QueryMatch& QueryIteration::Iterator::operator*() const noexcept {
    _iteration->_match.setEntity(_iteration->_cursor.getEntity());
    return _iteration->_match;
}

QueryIteration::Iterator& QueryIteration::Iterator::operator++() noexcept {
    _iteration->_cursor.advance();
    return *this;
}

void QueryIteration::Iterator::operator++(int) noexcept { ++*this; }

bool QueryIteration::Iterator::operator==(std::default_sentinel_t /*sentinel*/) const noexcept {
    return _iteration->_cursor.isDone();
}

QueryIteration::QueryIteration(const DynamicQuery& query, Tick lastRun, Tick thisRun)
    : _cursor{query.getTerms(), resolvePools(query),
              std::vector<std::uint32_t>(query.getTerms().size(), DynamicQuery::kNoPosition), lastRun, thisRun},
      _match{query, _cursor.getPools(), _cursor.getPositions(), thisRun} {}

QueryIteration::Iterator QueryIteration::begin() noexcept { return Iterator{*this}; }

std::default_sentinel_t QueryIteration::end() noexcept { return std::default_sentinel; }
}  // namespace rtype::ecs
