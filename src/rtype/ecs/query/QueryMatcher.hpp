/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryMatcher
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <span>

#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/QueryTerm.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"

// The join runs once per visited entity: it lives in this header, inline, so that a typed Query, whose terms are
// known at compile time, can have it inlined and simplified in its loop instead of calling into the library.
namespace rtype::ecs::detail {
/// @brief Position recorded for a term whose component the matched entity does not have (absent optional,
/// without).
inline constexpr std::uint32_t kNoPosition = std::numeric_limits<std::uint32_t>::max();

/// @return The index of the required term with the fewest entities: a query pass walks its pool.
/// @param terms Checked terms, with at least one required term (DynamicQuery::create() guarantees it).
/// @param pools The pool of each term, in term order.
[[nodiscard]] inline std::size_t findDriver(std::span<const QueryTerm> terms,
                                            std::span<storage::SparseSetStorage* const> pools) noexcept {
    std::size_t driver = 0;
    std::size_t driverSize = std::numeric_limits<std::size_t>::max();
    std::size_t index = 0;
    auto pool = pools.begin();
    for (const QueryTerm& term : terms) {
        if (isRequiredTerm(term.kind) && (*pool)->getSize() < driverSize) {
            driver = index;
            driverSize = (*pool)->getSize();
        }
        ++index;
        ++pool;
    }
    return driver;
}

/// @brief Checks every term against one entity of the driver pool: required pools contain it (with their tick
/// filters), without pools do not. The one join algorithm of every query.
/// @param driver The index of the driver term; its pool is not searched: the entity is at `driverPosition` in it.
/// @param driverPosition The entity's position in the driver pool, where the pass found it.
/// @param pools The pool of each term, in term order.
/// @param positions Filled with the entity's position in each term's pool, or kNoPosition.
/// @return Whether the entity matches; `positions` is only complete when it does.
[[nodiscard]] inline bool matchEntity(std::span<const QueryTerm> terms, Entity entity, std::size_t driver,
                                      std::uint32_t driverPosition, std::span<storage::SparseSetStorage* const> pools,
                                      std::span<std::uint32_t> positions, Tick lastRun, Tick thisRun) noexcept {
    std::size_t index = 0;
    auto pool = pools.begin();
    auto position = positions.begin();
    for (const QueryTerm& term : terms) {
        const storage::SparseSetStorage& storage = **pool;
        const std::optional<std::uint32_t> found =
            index == driver ? std::optional<std::uint32_t>{driverPosition} : storage.find(entity);
        *position = found.value_or(kNoPosition);
        if (term.kind == TermKind::kWithout) {
            if (found) {
                return false;
            }
        } else if (isRequiredTerm(term.kind)) {
            if (!found) {
                return false;
            }
            if (term.kind == TermKind::kAdded &&
                !isNewerThan(*std::next(storage.getAddedTicks().begin(), *found), lastRun, thisRun)) {
                return false;
            }
            if (term.kind == TermKind::kChanged &&
                !isNewerThan(*std::next(storage.getChangedTicks().begin(), *found), lastRun, thisRun)) {
                return false;
            }
        }
        ++index;
        ++pool;
        ++position;
    }
    return true;
}
}  // namespace rtype::ecs::detail
