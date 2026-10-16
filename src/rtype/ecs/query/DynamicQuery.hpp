/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DynamicQuery
*/

#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/query/QueryTerm.hpp"

namespace rtype::ecs {
/// @brief A query built from component ids: the single query engine, used by the Luau bridge, the network and the
/// typed Query.
///
/// @details Its terms are resolved and checked once, when it is created; afterwards only the iteration runs. It
/// keeps a pointer to the world's components, so it only ever iterates the world it was built for: the world must
/// outlive it. Build one with QueryBuilder.
class RTYPE_ECS_API DynamicQuery {
  public:
    /// @brief Checks the terms and builds the query.
    /// @details Rejected: an id not registered in `components`, no required term (read, write, with, added or
    /// changed: a query of optional terms only would match every entity), a component in two data terms (read,
    /// write and their optional forms), a component both excluded (without) and used by another term, and the same
    /// term twice. A data term may be combined with with, added or changed on the same component.
    /// @param components The components of the world the query iterates; they must outlive the query.
    /// @return The query, or an ErrorKind::kInvalidQuery error naming the faulty component.
    [[nodiscard]] static Result<DynamicQuery> create(Components& components, std::vector<QueryTerm> terms);

    ~DynamicQuery() = default;
    DynamicQuery(const DynamicQuery& other) = default;
    DynamicQuery& operator=(const DynamicQuery& other) = default;
    DynamicQuery(DynamicQuery&& other) noexcept = default;
    DynamicQuery& operator=(DynamicQuery&& other) noexcept = default;

    /// @return The terms, in the order they were given.
    [[nodiscard]] std::span<const QueryTerm> getTerms() const noexcept;

    /// @return The number of data terms (read, write and their optional forms): a match exposes one value per data
    /// term, numbered in the order the terms were given.
    [[nodiscard]] std::size_t getDataTermCount() const noexcept;

    /// @return Each component whose values the query reads or writes, once (written wins over read). With and
    /// without access no value; added and changed read the ticks, so they count as reads.
    [[nodiscard]] std::span<const ComponentAccess> getAccess() const noexcept;

  private:
    DynamicQuery(Components& components, std::vector<QueryTerm> terms);

    Components* _components;               ///< The world's components; the world must outlive the query.
    std::vector<QueryTerm> _terms;         ///< Every term, in the order given.
    std::vector<std::size_t> _dataTerms;   ///< Index in _terms of each data term, in order.
    std::vector<ComponentAccess> _access;  ///< One entry per accessed component.
};
}  // namespace rtype::ecs
