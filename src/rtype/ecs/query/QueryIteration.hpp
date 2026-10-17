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
#include "rtype/ecs/query/QueryCursor.hpp"
#include "rtype/ecs/query/QueryMatch.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"

namespace rtype::ecs {
class DynamicQuery;

/// @brief One pass of a DynamicQuery over its world, used in a range-based for loop.
///
/// @details Made by DynamicQuery::iter(). It walks the dense entities of the smallest required pool (the driver),
/// through a detail::QueryCursor shared with the typed Query, and yields the QueryMatch of every entity that satisfies
/// all the terms. Its pool and position buffers are allocated once, when it is made; stepping never allocates. Adding
/// or removing components of a queried pool, or despawning, during the loop invalidates it: defer such changes until
/// after the loop.
/// @code
/// for (rtype::ecs::QueryMatch& match : query.iter(lastRun)) {
///     std::span<std::byte> shield = match.getMut(0);
/// }
/// @endcode
class RTYPE_ECS_API QueryIteration {
  public:
    /// @brief Steps the pass's cursor and shows its current match.
    class RTYPE_ECS_API Iterator {
      public:
        using iterator_concept = std::input_iterator_tag;  ///< Single pass: it satisfies std::input_iterator.
        using difference_type = std::ptrdiff_t;
        using value_type = QueryMatch;

        /// @brief An iterator on the pass's current match.
        explicit Iterator(QueryIteration& iteration) noexcept;
        ~Iterator() = default;
        Iterator(const Iterator& other) noexcept = default;
        Iterator& operator=(const Iterator& other) noexcept = default;
        Iterator(Iterator&& other) noexcept = default;
        Iterator& operator=(Iterator&& other) noexcept = default;

        /// @return The current match.
        [[nodiscard]] QueryMatch& operator*() const noexcept;

        /// @brief Moves to the next match.
        Iterator& operator++() noexcept;

        /// @brief Moves to the next match; returns nothing, as single-pass iterators may.
        void operator++(int) noexcept;

        /// @return Whether every entity of the driver pool has been visited.
        [[nodiscard]] bool operator==(std::default_sentinel_t sentinel) const noexcept;

      private:
        QueryIteration* _iteration;  ///< The pass this iterator steps.
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

    /// @return An iterator on the first match. The pass is single: a second begin() continues where it is.
    [[nodiscard]] Iterator begin() noexcept;

    /// @return The end of the pass.
    [[nodiscard]] static std::default_sentinel_t end() noexcept;

  private:
    /// @brief The walk over the driver pool; its vectors are allocated once, when the pass starts.
    detail::QueryCursor<std::vector<storage::SparseSetStorage*>, std::vector<std::uint32_t>> _cursor;
    QueryMatch _match;  ///< The current match: views the cursor's buffers, which a move keeps in place.
};
}  // namespace rtype::ecs
