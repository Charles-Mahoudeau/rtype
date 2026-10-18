/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Query
*/

#pragma once

#include <iterator>
#include <optional>

#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/query/QueryState.hpp"
#include "rtype/ecs/query/TermTraits.hpp"

namespace rtype::ecs {
/// @brief What a system's body receives for a query: a light view of the system's QueryState and the tick the
/// system last ran at, iterated directly.
///
/// @details It is made for each run and passed by value: it holds a pointer to the state and a tick, plus the pass in
/// progress (inline, no allocation). Each begin() starts a new pass over the world; moving the view while iterating
/// invalidates the pass. See QueryState for what each term yields.
/// @code
/// void movement(rtype::ecs::Query<Position, const Velocity> query, rtype::ecs::Res<Time> time) {
///     for (auto [entity, position, velocity] : query) {
///         position->x += velocity.x * time->fixedDelta;
///     }
/// }
/// @endcode
template <typename... Terms>
    requires ValidQuery<Terms...>
class Query {
  public:
    using State = QueryState<Terms...>;           ///< The query built once, kept by the system.
    using Item = State::Item;                     ///< What one match yields: the entity, then one item per data term.
    using Iterator = State::Iteration::Iterator;  ///< What begin() returns: walks the matches of one pass.

    /// @param state The query built once; it must outlive the view.
    /// @param lastRun Tick the system last ran at: added and changed filters keep values stamped after it.
    Query(const State& state, Tick lastRun) noexcept;
    ~Query() = default;
    Query(const Query& other) = delete;
    Query& operator=(const Query& other) = delete;
    Query(Query&& other) noexcept = default;
    Query& operator=(Query&& other) noexcept = default;

    /// @return The tick the system last ran at.
    [[nodiscard]] Tick getLastRun() const noexcept;

    /// @return An iterator on the first match of a new pass over the world.
    [[nodiscard]] Iterator begin() noexcept;

    /// @return The end of the pass.
    [[nodiscard]] static std::default_sentinel_t end() noexcept;

  private:
    const State* _state;                             ///< The query built once.
    Tick _lastRun;                                   ///< Tick the system last ran at.
    std::optional<typename State::Iteration> _pass;  ///< The pass begin() started, kept for its iterators.
};
}  // namespace rtype::ecs

#include "Query.tpp"
