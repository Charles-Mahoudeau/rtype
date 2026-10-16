/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryMatch
*/

#include "QueryMatch.hpp"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <span>

#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/DynamicQuery.hpp"
#include "rtype/ecs/query/QueryTerm.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"

namespace rtype::ecs {
QueryMatch::QueryMatch(const DynamicQuery& query, std::span<storage::SparseSetStorage* const> pools,
                       std::span<const std::uint32_t> positions, Tick thisRun) noexcept
    : _query{&query}, _pools{pools}, _positions{positions}, _thisRun{thisRun} {}

Entity QueryMatch::getEntity() const noexcept { return _entity; }

void QueryMatch::setEntity(Entity entity) noexcept { _entity = entity; }

std::span<const std::byte> QueryMatch::get(std::size_t index) const noexcept {
    const std::optional<std::size_t> term = findTerm(index);
    if (!term) {
        return {};
    }
    const std::uint32_t position = *std::next(_positions.begin(), static_cast<std::ptrdiff_t>(*term));
    if (position == DynamicQuery::kNoPosition) {
        return {};
    }
    const storage::SparseSetStorage& pool = **std::next(_pools.begin(), static_cast<std::ptrdiff_t>(*term));
    return pool.at(position);
}

std::span<std::byte> QueryMatch::getMut(std::size_t index) noexcept {
    const std::optional<std::size_t> term = findTerm(index);
    if (!term || !isWriteTerm(std::next(_query->getTerms().begin(), static_cast<std::ptrdiff_t>(*term))->kind)) {
        return {};
    }
    const std::uint32_t position = *std::next(_positions.begin(), static_cast<std::ptrdiff_t>(*term));
    if (position == DynamicQuery::kNoPosition) {
        return {};
    }
    storage::SparseSetStorage& pool = **std::next(_pools.begin(), static_cast<std::ptrdiff_t>(*term));
    *std::next(pool.getChangedTicks().begin(), position) = _thisRun;
    return pool.at(position);
}

std::optional<std::size_t> QueryMatch::findTerm(std::size_t index) const noexcept {
    const std::span<const std::size_t> dataTerms = _query->getDataTerms();
    if (index >= dataTerms.size()) {
        return std::nullopt;
    }
    return *std::next(dataTerms.begin(), static_cast<std::ptrdiff_t>(index));
}
}  // namespace rtype::ecs
