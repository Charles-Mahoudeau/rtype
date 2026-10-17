/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryMatcher
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/QueryTerm.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"

namespace rtype::ecs::detail {
/// @brief Position recorded for a term whose component the matched entity does not have (absent optional,
/// without).
inline constexpr std::uint32_t kNoPosition = std::numeric_limits<std::uint32_t>::max();

/// @return The index of the required term with the fewest entities: a query pass walks its pool.
/// @param terms Checked terms, with at least one required term (DynamicQuery::create() guarantees it).
/// @param pools The pool of each term, in term order.
[[nodiscard]] RTYPE_ECS_API std::size_t findDriver(std::span<const QueryTerm> terms,
                                                   std::span<storage::SparseSetStorage* const> pools) noexcept;

/// @brief Checks every term against one entity: required pools contain it (with their tick filters), without pools
/// do not. The one join algorithm of every query.
/// @param pools The pool of each term, in term order.
/// @param positions Filled with the entity's position in each term's pool, or kNoPosition.
/// @return Whether the entity matches; `positions` is only complete when it does.
[[nodiscard]] RTYPE_ECS_API bool matchEntity(std::span<const QueryTerm> terms, Entity entity,
                                             std::span<storage::SparseSetStorage* const> pools,
                                             std::span<std::uint32_t> positions, Tick lastRun, Tick thisRun) noexcept;
}  // namespace rtype::ecs::detail
