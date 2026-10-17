/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryMatch
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"

namespace rtype::ecs {
class DynamicQuery;

/// @brief One entity matched by a DynamicQuery, and access to its values: one per data term, numbered in the order
/// the data terms were given (`get(0)` is the first read, write or optional term).
///
/// @details Valid until the iteration moves to the next entity. Its values are invalidated, like every pool access,
/// by adding or removing components of a queried pool.
class RTYPE_ECS_API QueryMatch {
  public:
    /// @param query The query being iterated.
    /// @param pools The pool of each term, in term order.
    /// @param positions The entity's position in each term's pool, in term order; DynamicQuery::kNoPosition when
    /// absent.
    /// @param thisRun The tick written values are stamped with.
    QueryMatch(const DynamicQuery& query, std::span<storage::SparseSetStorage* const> pools,
               std::span<const std::uint32_t> positions, Tick thisRun) noexcept;
    ~QueryMatch() = default;
    QueryMatch(const QueryMatch& other) = delete;
    QueryMatch& operator=(const QueryMatch& other) = delete;
    QueryMatch(QueryMatch&& other) noexcept = default;
    QueryMatch& operator=(QueryMatch&& other) noexcept = default;

    /// @return The matched entity.
    [[nodiscard]] Entity getEntity() const noexcept;

    /// @brief Sets the matched entity. Called by the iteration.
    void setEntity(Entity entity) noexcept;

    /// @return The bytes of the value of data term `index`; empty if it is an optional term the entity lacks, a
    /// tag, or `index` is not a data term. Never marks the value changed.
    [[nodiscard]] std::span<const std::byte> get(std::size_t index) const noexcept;

    /// @brief Write access to the value of data term `index`, marked changed now: the caller only has bytes, so it
    /// cannot tell a read from a write. Use get() to only read.
    /// @return The bytes of the value; empty if the term is not a write term, the entity lacks an optional
    /// component, it is a tag, or `index` is not a data term.
    [[nodiscard]] std::span<std::byte> getMut(std::size_t index) noexcept;

  private:
    /// @return The index in the query's terms of data term `index`, or nothing if there is no such data term.
    [[nodiscard]] std::optional<std::size_t> findTerm(std::size_t index) const noexcept;

    const DynamicQuery* _query;                          ///< The query being iterated.
    std::span<storage::SparseSetStorage* const> _pools;  ///< The pool of each term, in term order.
    std::span<const std::uint32_t> _positions;           ///< The entity's position in each term's pool.
    Tick _thisRun;                                       ///< Tick stamped by getMut().
    Entity _entity;                                      ///< The matched entity.
};
}  // namespace rtype::ecs
