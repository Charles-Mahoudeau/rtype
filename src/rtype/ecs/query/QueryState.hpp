/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryState
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <tuple>
#include <utility>

#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/DynamicQuery.hpp"
#include "rtype/ecs/query/QueryCursor.hpp"
#include "rtype/ecs/query/QueryTerm.hpp"
#include "rtype/ecs/query/TermTraits.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"

namespace rtype::ecs {
namespace detail {
/// @brief `std::tuple<Item>` for a term that yields one, `std::tuple<>` for a filter.
template <TypedTerm Term, bool = TermTraits<Term>::kHasItem>
struct ItemTuple {
    using type = std::tuple<>;  ///< Filters yield nothing.
};

/// @brief `std::tuple<Item>` for a term that yields one.
template <TypedTerm Term>
struct ItemTuple<Term, true> {
    using type = std::tuple<typename TermTraits<Term>::Item>;  ///< The term's item.
};
}  // namespace detail

/// @brief A query written with C++ types, built once: its resolved terms, and the passes over the world.
///
/// @details A system keeps one per Query parameter and hands its body a Query view of it at each run. `T` yields a
/// Mut<T> (marks the value changed only when written through), `const T` a `const T&`, `Optional<T>` a Mut<T> that may
/// be empty, `Optional<const T>` a `const T*` that may be null; the filters With, Without, Added and Changed yield
/// nothing. Each match is a tuple of the entity and those items, in term order, made for structured bindings. It
/// compiles down to a DynamicQuery, built once; terms that cannot form a query (two writes of one component, only
/// optional terms...) do not compile. It keeps a pointer to the world's components: the world must outlive it.
/// @code
/// rtype::ecs::QueryState<Position, const Velocity, rtype::ecs::Without<Dead>> query{world.getComponents()};
/// for (auto [entity, position, velocity] : query.iter(lastRun)) {
///     position->x += velocity.x;
/// }
/// @endcode
template <typename... Terms>
    requires ValidQuery<Terms...>
class QueryState {
  public:
    /// @brief What one match yields: the entity, then the item of every term that has one, in term order.
    using Item = decltype(std::tuple_cat(std::declval<std::tuple<Entity>>(),
                                         std::declval<typename detail::ItemTuple<Terms>::type>()...));

    /// @brief One pass over the world's matching entities. Its buffers live in the object itself: iterating never
    /// allocates.
    class Iteration {
      public:
        /// @brief Steps the pass's cursor and builds its current match.
        class Iterator {
          public:
            using iterator_concept = std::input_iterator_tag;  ///< Single pass: it satisfies std::input_iterator.
            using difference_type = std::ptrdiff_t;
            using value_type = Item;

            /// @brief An iterator on the pass's current match.
            explicit Iterator(Iteration& iteration) noexcept;
            ~Iterator() = default;
            Iterator(const Iterator& other) noexcept = default;
            Iterator& operator=(const Iterator& other) noexcept = default;
            Iterator(Iterator&& other) noexcept = default;
            Iterator& operator=(Iterator&& other) noexcept = default;

            /// @return The current match, by value: bind it with `auto [entity, ...]`.
            [[nodiscard]] Item operator*() const noexcept;

            /// @brief Moves to the next match.
            Iterator& operator++() noexcept;

            /// @brief Moves to the next match; returns nothing, as single-pass iterators may.
            void operator++(int) noexcept;

            /// @return Whether every entity of the driver pool has been visited.
            [[nodiscard]] bool operator==(std::default_sentinel_t sentinel) const noexcept;

          private:
            Iteration* _iteration;  ///< The pass this iterator steps.
        };

        /// @param query The dynamic query to run; it must outlive the iteration.
        /// @param lastRun Tick the query last ran at.
        /// @param thisRun The world's change tick now, stamped on values written through a Mut.
        Iteration(const DynamicQuery& query, Tick lastRun, Tick thisRun) noexcept;
        ~Iteration() = default;
        Iteration(const Iteration& other) = delete;
        Iteration& operator=(const Iteration& other) = delete;
        Iteration(Iteration&& other) noexcept = default;
        Iteration& operator=(Iteration&& other) noexcept = default;

        /// @return An iterator on the first match. The pass is single: a second begin() continues where it is.
        [[nodiscard]] Iterator begin() noexcept;

        /// @return The end of the pass.
        [[nodiscard]] static std::default_sentinel_t end() noexcept;

      private:
        /// @return The pool of every term of `query`, in term order.
        [[nodiscard]] static std::array<storage::SparseSetStorage*, sizeof...(Terms)> resolvePools(
            const DynamicQuery& query) noexcept;

        /// @return The current entity's items, in term order.
        template <std::size_t... Indices>
        [[nodiscard]] Item makeItem(std::index_sequence<Indices...> indices) const noexcept;

        /// @return The item of term `Index` for the current entity, in a tuple; an empty tuple for a filter.
        template <std::size_t Index>
        [[nodiscard]] auto fetch() const noexcept;

        /// @brief The walk over the driver pool, with its buffers in `std::array`s.
        detail::QueryCursor<std::array<storage::SparseSetStorage*, sizeof...(Terms)>,
                            std::array<std::uint32_t, sizeof...(Terms)>>
            _cursor;
    };

    /// @brief Builds the query, registering with the default options every term's component not registered yet.
    /// @param components The components of the world the query iterates; they must outlive the query.
    /// @throws exceptions::ComponentRegistrationException When a component must be registered and its name is taken
    /// (by another layout or a resource).
    explicit QueryState(Components& components);
    ~QueryState() = default;
    QueryState(const QueryState& other) = default;
    QueryState& operator=(const QueryState& other) = default;
    QueryState(QueryState&& other) noexcept = default;
    QueryState& operator=(QueryState&& other) noexcept = default;

    /// @return The dynamic query this one compiles down to.
    [[nodiscard]] const DynamicQuery& getDynamic() const noexcept;

    /// @return Each component whose values the query reads or writes: what the scheduler orders systems by.
    [[nodiscard]] std::span<const ComponentAccess> getAccess() const noexcept;

    /// @brief Starts a pass over the world's matching entities; see DynamicQuery::iter() for the ticks.
    [[nodiscard]] Iteration iter(Tick lastRun) const noexcept;

  private:
    /// @return The dynamic query of the terms, registering their components if needed.
    [[nodiscard]] static DynamicQuery build(Components& components);

    /// @return The id of `T` in `components`, registering it with the default options if needed.
    template <Component T>
    [[nodiscard]] static ComponentId idOf(Components& components);

    Components* _components;  ///< The world's components, read for the change tick.
    DynamicQuery _query;      ///< The dynamic query this one compiles down to.
};
}  // namespace rtype::ecs

#include "QueryState.tpp"
