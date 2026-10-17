/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryCursor
*/

#pragma once

#include <cstddef>
#include <span>

#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/QueryTerm.hpp"

namespace rtype::ecs::detail {
/// @brief The walk shared by every query pass: it moves along the dense entities of the smallest required pool (the
/// driver, from findDriver()) and stops on each entity matchEntity() accepts.
///
/// @details The buffers are template parameters so that each pass keeps them where it wants: `std::vector`s for a
/// DynamicQuery (its term count is only known at run time), `std::array`s for a typed Query (no allocation). It owns
/// them, so moving the cursor keeps them valid. A cursor makes one pass: it starts on the first match.
/// @tparam Pools A container of one `storage::SparseSetStorage*` per term, already filled.
/// @tparam Positions A container of one `std::uint32_t` per term, already sized.
template <typename Pools, typename Positions>
class QueryCursor {
  public:
    /// @param terms The checked terms of the query (DynamicQuery::getTerms()); they must outlive the cursor.
    /// @param pools The pool of each term, in term order (DynamicQuery::resolvePools()).
    /// @param positions One slot per term, filled at each match with the entity's position in the term's pool.
    /// @param lastRun Tick the query last ran at: added and changed filters keep values stamped after it.
    /// @param thisRun The world's change tick now, stamped on written values.
    QueryCursor(std::span<const QueryTerm> terms, Pools pools, Positions positions, Tick lastRun,
                Tick thisRun) noexcept;
    ~QueryCursor() = default;
    QueryCursor(const QueryCursor& other) = default;
    QueryCursor& operator=(const QueryCursor& other) = default;
    QueryCursor(QueryCursor&& other) noexcept = default;
    QueryCursor& operator=(QueryCursor&& other) noexcept = default;

    /// @return The pool of each term, in term order.
    [[nodiscard]] const Pools& getPools() const noexcept;

    /// @return The current entity's position in each term's pool, in term order; DynamicQuery::kNoPosition when
    /// absent.
    [[nodiscard]] const Positions& getPositions() const noexcept;

    /// @return The world's change tick when the pass started: the stamp of written values.
    [[nodiscard]] Tick getThisRun() const noexcept;

    /// @return The current matching entity. Only meaningful while isDone() is false.
    [[nodiscard]] Entity getEntity() const noexcept;

    /// @return Whether every entity of the driver pool has been visited. The size is read again at each call, so a
    /// pool that shrank under the loop ends it instead of being overrun.
    [[nodiscard]] bool isDone() const noexcept;

    /// @brief Moves to the next matching entity, or to the end.
    void advance() noexcept;

  private:
    /// @brief Moves `_index` forward to the first matching entity at or after it, or to the end.
    void skipToMatch() noexcept;

    std::span<const QueryTerm> _terms;  ///< The query's terms.
    Pools _pools;                       ///< The pool of each term, in term order.
    Positions _positions;               ///< The current entity's position in each term's pool.
    std::size_t _driver;                ///< Index of the term whose pool drives the pass.
    Tick _lastRun;                      ///< Tick the query last ran at.
    Tick _thisRun;                      ///< The world's change tick when the pass started.
    std::size_t _index{0};              ///< Position in the driver pool's dense entities.
    Entity _entity;                     ///< The current matching entity.
};
}  // namespace rtype::ecs::detail

#include "QueryCursor.tpp"
