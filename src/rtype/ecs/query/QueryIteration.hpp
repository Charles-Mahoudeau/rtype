/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryIteration
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/query/QueryMatch.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"

namespace rtype::ecs {
class DynamicQuery;

/// @brief One pass of a DynamicQuery over its world, used in a range-based for loop.
///
/// @details Made by DynamicQuery::iter(). It walks the dense entities of the smallest required pool (the driver) and
/// yields the QueryMatch of every entity that satisfies all the terms. Its pool and position buffers are allocated
/// once, when it is made; stepping never allocates. Adding or removing components of a queried pool, or despawning,
/// during the loop invalidates it: defer such changes until after the loop.
/// @code
/// for (rtype::ecs::QueryMatch& match : query.iter(lastRun)) {
///     std::span<std::byte> shield = match.getMut(0);
/// }
/// @endcode
class RTYPE_ECS_API QueryIteration {
  public:
    /// @brief Walks the driver pool, stopping on matching entities.
    class RTYPE_ECS_API Iterator {
      public:
        using difference_type = std::ptrdiff_t;
        using value_type = QueryMatch;

        /// @brief An iterator on the first match at or after `index`.
        Iterator(QueryIteration& iteration, std::size_t index) noexcept;
        ~Iterator() = default;
        Iterator(const Iterator& other) noexcept = default;
        Iterator& operator=(const Iterator& other) noexcept = default;
        Iterator(Iterator&& other) noexcept = default;
        Iterator& operator=(Iterator&& other) noexcept = default;

        /// @return The current match.
        [[nodiscard]] QueryMatch& operator*() const noexcept;

        /// @brief Moves to the next match.
        Iterator& operator++() noexcept;

        /// @return Whether every entity of the driver pool has been visited.
        [[nodiscard]] bool operator==(std::default_sentinel_t sentinel) const noexcept;

      private:
        /// @brief Moves `_index` forward to the first matching entity, or to the end.
        void skipToMatch() noexcept;

        QueryIteration* _iteration;  ///< The pass this iterator walks.
        std::size_t _index;          ///< Position in the driver pool's dense entities.
    };

    /// @param query The query to run; it must outlive the iteration.
    /// @param lastRun Tick the query last ran at: added and changed filters keep values stamped after it.
    /// @param thisRun The world's change tick now, stamped on written values.
    QueryIteration(const DynamicQuery& query, Tick lastRun, Tick thisRun);
    ~QueryIteration() = default;
    QueryIteration(const QueryIteration& other) = delete;
    QueryIteration& operator=(const QueryIteration& other) = delete;
    QueryIteration(QueryIteration&& other) noexcept = default;
    QueryIteration& operator=(QueryIteration&& other) noexcept = default;

    /// @return An iterator on the first match.
    [[nodiscard]] Iterator begin() noexcept;

    /// @return The end of the pass.
    [[nodiscard]] static std::default_sentinel_t end() noexcept;

  private:
    const DynamicQuery* _query;                      ///< The query being run.
    std::vector<storage::SparseSetStorage*> _pools;  ///< The pool of each term, in term order.
    std::vector<std::uint32_t> _positions;           ///< The current entity's position in each term's pool.
    std::size_t _driver;                             ///< Index of the term whose pool drives the pass.
    Tick _lastRun;                                   ///< Tick the query last ran at.
    Tick _thisRun;                                   ///< The world's change tick when the pass started.
    QueryMatch _match;                               ///< The current match, refreshed at each step.
};
}  // namespace rtype::ecs
