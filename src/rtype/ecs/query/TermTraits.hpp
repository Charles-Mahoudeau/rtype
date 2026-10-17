/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TermTraits
*/

#pragma once

#include <cstdint>

#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/query/QueryTerm.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"

namespace rtype::ecs {
/// @brief What a C++ query term means: its component, its TermKind, and what the iteration yields for it.
///
/// @details Only the specializations below exist: `T` (written, yields Mut<T>), `const T` (read, yields `const T&`),
/// `Optional<T>` (yields a Mut<T> that may be empty), `Optional<const T>` (yields a `const T*` that may be null), and
/// the filters `With<T>`, `Without<T>`, `Added<T>`, `Changed<T>` (yield nothing). `T` must be a Component.
template <typename Term>
struct TermTraits;

namespace detail {
/// @brief The traits shared by the filters: a component, a kind, and nothing yielded.
template <Component T, TermKind Kind>
struct FilterTraits {
    using Type = T;  ///< The component.

    static constexpr TermKind kKind = Kind;  ///< What the term asks of the component.
    static constexpr bool kHasItem = false;  ///< Filters yield nothing.
};
}  // namespace detail

/// @brief `T`: required, written; yields a Mut<T>, which marks the value changed only when written through.
template <Component T>
struct TermTraits<T> {
    using Type = T;       ///< The component.
    using Item = Mut<T>;  ///< What the iteration yields.

    static constexpr TermKind kKind = TermKind::kWrite;  ///< What the term asks of the component.
    static constexpr bool kHasItem = true;               ///< The term yields an Item.

    /// @return The value at `position` in `pool`, stamped with `thisRun` when written through.
    [[nodiscard]] static Item fetch(storage::SparseSetStorage& pool, std::uint32_t position, Tick thisRun) noexcept;
};

/// @brief `const T`: required, read; yields `const T&`.
template <Component T>
struct TermTraits<const T> {
    using Type = T;         ///< The component.
    using Item = const T&;  ///< What the iteration yields.

    static constexpr TermKind kKind = TermKind::kRead;  ///< What the term asks of the component.
    static constexpr bool kHasItem = true;              ///< The term yields an Item.

    /// @return The value at `position` in `pool`.
    [[nodiscard]] static Item fetch(storage::SparseSetStorage& pool, std::uint32_t position, Tick thisRun) noexcept;
};

/// @brief `Optional<T>`: may be absent, written; yields a Mut<T>, empty when the entity lacks the component.
template <Component T>
struct TermTraits<Optional<T>> {
    using Type = T;       ///< The component.
    using Item = Mut<T>;  ///< What the iteration yields.

    static constexpr TermKind kKind = TermKind::kOptionalWrite;  ///< What the term asks of the component.
    static constexpr bool kHasItem = true;                       ///< The term yields an Item.

    /// @return The value at `position` in `pool`, or an empty Mut when `position` is DynamicQuery::kNoPosition.
    [[nodiscard]] static Item fetch(storage::SparseSetStorage& pool, std::uint32_t position, Tick thisRun) noexcept;
};

/// @brief `Optional<const T>`: may be absent, read; yields a `const T*`, null when the entity lacks the component.
template <Component T>
struct TermTraits<Optional<const T>> {
    using Type = T;         ///< The component.
    using Item = const T*;  ///< What the iteration yields.

    static constexpr TermKind kKind = TermKind::kOptionalRead;  ///< What the term asks of the component.
    static constexpr bool kHasItem = true;                      ///< The term yields an Item.

    /// @return The value at `position` in `pool`, or nullptr when `position` is DynamicQuery::kNoPosition.
    [[nodiscard]] static Item fetch(storage::SparseSetStorage& pool, std::uint32_t position, Tick thisRun) noexcept;
};

/// @brief `With<T>`: required, not accessed.
template <Component T>
struct TermTraits<With<T>> : detail::FilterTraits<T, TermKind::kWith> {};

/// @brief `Without<T>`: must be absent.
template <Component T>
struct TermTraits<Without<T>> : detail::FilterTraits<T, TermKind::kWithout> {};

/// @brief `Added<T>`: required, added since the query last ran.
template <Component T>
struct TermTraits<Added<T>> : detail::FilterTraits<T, TermKind::kAdded> {};

/// @brief `Changed<T>`: required, added or written since the query last ran.
template <Component T>
struct TermTraits<Changed<T>> : detail::FilterTraits<T, TermKind::kChanged> {};

/// @brief A C++ type usable as a query term: a component, `const` component, Optional, With, Without, Added or
/// Changed of a component.
template <typename Term>
concept TypedTerm = requires { TermTraits<Term>::kKind; };

namespace detail {
/// @return Whether terms can form one query: at least one required term, no data term on a tag, and no two terms on
/// the same component that findConflict() refuses. The same rules as DynamicQuery::create(), checked at compile time.
template <TypedTerm... Terms>
consteval bool areCompatibleTerms() noexcept;
}  // namespace detail

/// @brief Terms that form a valid typed query; Query<Terms...> requires it, so a wrong query does not compile.
/// @code
/// static_assert(rtype::ecs::ValidQuery<Position, const Velocity, rtype::ecs::Without<Dead>>);
/// static_assert(!rtype::ecs::ValidQuery<Position, Position>);  // two writes of one component
/// @endcode
template <typename... Terms>
concept ValidQuery = (TypedTerm<Terms> && ...) && detail::areCompatibleTerms<Terms...>();
}  // namespace rtype::ecs

#include "TermTraits.tpp"
