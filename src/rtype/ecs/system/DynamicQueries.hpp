/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DynamicQueries
*/

#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/query/DynamicQuery.hpp"
#include "rtype/ecs/query/QueryIteration.hpp"
#include "rtype/ecs/system/QueryList.hpp"
#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/system/SystemParam.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
/// @brief A system parameter running queries chosen when the system is added, by component id: for components the
/// system does not know at compile time (the replicated ones, those a script declares).
///
/// @details Configured with a QueryList (see SystemParam): each builder becomes one query, built once when the system
/// is added, and the system's access set gets every component of every query, so the scheduler orders it and reports
/// ambiguities as for a typed Query. Each run gets a view of the queries with the system's last run: iter() reports
/// added and changed values since then, iterSince() since any tick (a client's acknowledged tick). A term declared
/// read refuses write access (QueryMatch::getMut()), so the system writes nothing it did not declare.
/// @code
/// void snapshot(rtype::ecs::Res<ServerState> state, rtype::ecs::DynamicQueries replicated) {
///     for (std::size_t query = 0; query < replicated.getCount(); ++query) {
///         for (rtype::ecs::QueryMatch& match : replicated.iter(query)) {
///             // match.getEntity(), match.get(0)...
///         }
///     }
/// }
/// app.addSystem(rtype::ecs::Phase::kPostUpdate, "net.snapshot", &snapshot, rtype::ecs::QueryList{builders});
/// @endcode
class RTYPE_ECS_API DynamicQueries {
  public:
    /// @param queries The queries, built when the system was added; they must outlive the view.
    /// @param lastRun Tick the system last ran at.
    DynamicQueries(std::span<const DynamicQuery> queries, Tick lastRun) noexcept;
    ~DynamicQueries() = default;
    DynamicQueries(const DynamicQueries& other) noexcept = default;
    DynamicQueries& operator=(const DynamicQueries& other) noexcept = default;
    DynamicQueries(DynamicQueries&& other) noexcept = default;
    DynamicQueries& operator=(DynamicQueries&& other) noexcept = default;

    /// @return The number of queries, as given in the QueryList.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @return The query at `index` (the index of its builder in the QueryList); `index` must be below getCount().
    [[nodiscard]] const DynamicQuery& getQuery(std::size_t index) const noexcept;

    /// @return Tick the system last ran at.
    [[nodiscard]] Tick getLastRun() const noexcept;

    /// @return The matches of the query at `index`, its added and changed terms counted since the system last ran.
    [[nodiscard]] QueryIteration iter(std::size_t index) const;

    /// @return The matches of the query at `index`, its added and changed terms counted since `since`.
    [[nodiscard]] QueryIteration iterSince(std::size_t index, Tick since) const;

  private:
    std::span<const DynamicQuery> _queries;  ///< The system's queries.
    Tick _lastRun;                           ///< Tick the system last ran at.
};

/// @brief `DynamicQueries`: configured with a QueryList; the system keeps the built queries.
template <>
struct RTYPE_ECS_API SystemParam<DynamicQueries> {
    using Config = QueryList;                 ///< The queries to build, given when the system is added.
    using State = std::vector<DynamicQuery>;  ///< The built queries, in the order of the QueryList.

    /// @return Every query of `config`, built from the world's components.
    /// @throws exceptions::QueryException When a query cannot be built (a component that is not registered, terms
    /// that contradict each other), naming the query and the component.
    [[nodiscard]] static State init(World& world, Config config);

    /// @return A view of the queries for this run.
    [[nodiscard]] static DynamicQueries get(State& state, const SystemContext& context) noexcept;

    /// @brief Adds the components every query reads and writes.
    static void addAccess(const State& state, SystemAccess& access);
};
}  // namespace rtype::ecs
