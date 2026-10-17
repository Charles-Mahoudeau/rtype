/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryTerm
*/

#pragma once

#include <cstdint>

#include "rtype/ecs/component/ComponentInfo.hpp"

namespace rtype::ecs {
/// @brief What a query term asks of one component.
/// @details A query is a list of terms, and each term is one condition on one component: "the entity must have
/// `Position` and the query writes it", "the entity must not have `Dead`", "`Health` changed since the last run".
/// An entity matches the query when it satisfies every term. Data terms (read, write and their optional forms) also
/// give the query access to the component's value; the others (with, without, added, changed) are filters that only
/// decide which entities match.
/// @code
/// // Query<Position, const Velocity, Without<Dead>> is three terms:
/// // {Position, kWrite}, {Velocity, kRead}, {Dead, kWithout}
/// @endcode
enum class TermKind : std::uint8_t {
    kRead,           ///< The entity must have it; its value is read.
    kWrite,          ///< The entity must have it; its value is written.
    kOptionalRead,   ///< The entity may have it; its value is read when present.
    kOptionalWrite,  ///< The entity may have it; its value is written when present.
    kWith,           ///< The entity must have it; its value is not accessed.
    kWithout,        ///< The entity must not have it.
    kAdded,          ///< The entity must have it, added since the query last ran.
    kChanged,        ///< The entity must have it, added or written since the query last ran.
};

/// @brief Why two terms on the same component cannot be in one query.
enum class TermConflict : std::uint8_t {
    kNone,          ///< They can: a data term with with, added or changed, or added with changed.
    kSameTerm,      ///< The same term twice (two writes, with twice...).
    kTwoDataTerms,  ///< Two data terms (a read and a write...): which one gives the value?
    kExcluded,      ///< One is without: the query could never match.
};

/// @brief One term of a query: a component, and what the query asks of it (see TermKind).
struct QueryTerm {
    ComponentId id{0};               ///< The component.
    TermKind kind{TermKind::kRead};  ///< What the query asks of it.

    [[nodiscard]] constexpr bool operator==(const QueryTerm& other) const noexcept = default;
};

/// @brief How a query accesses one component's values: what the scheduler orders systems by.
struct ComponentAccess {
    ComponentId id{0};  ///< The component.
    bool write{false};  ///< True when the query writes the values, false when it only reads them (or their ticks).

    [[nodiscard]] constexpr bool operator==(const ComponentAccess& other) const noexcept = default;
};

// The functions below are constexpr and defined here, not in a .cpp: the typed Query checks its terms at compile
// time with them (ValidQuery, in TermTraits.hpp), and DynamicQuery::create() checks them at run time with the same
// functions, so both kinds of query follow one set of rules.

/// @return Whether the term gives access to the component's value (read, write and their optional forms).
[[nodiscard]] constexpr bool isDataTerm(TermKind kind) noexcept {
    return kind == TermKind::kRead || kind == TermKind::kWrite || kind == TermKind::kOptionalRead ||
           kind == TermKind::kOptionalWrite;
}

/// @return Whether the term requires the entity to have the component: such a term can drive the iteration.
[[nodiscard]] constexpr bool isRequiredTerm(TermKind kind) noexcept {
    return kind == TermKind::kRead || kind == TermKind::kWrite || kind == TermKind::kWith || kind == TermKind::kAdded ||
           kind == TermKind::kChanged;
}

/// @return Whether the term writes the component's value.
[[nodiscard]] constexpr bool isWriteTerm(TermKind kind) noexcept {
    return kind == TermKind::kWrite || kind == TermKind::kOptionalWrite;
}

/// @return Why two terms on the same component cannot be in one query; the rule shared by dynamic and typed queries.
[[nodiscard]] constexpr TermConflict findConflict(TermKind first, TermKind second) noexcept {
    if (first == second) {
        return TermConflict::kSameTerm;
    }
    if (isDataTerm(first) && isDataTerm(second)) {
        return TermConflict::kTwoDataTerms;
    }
    if (first == TermKind::kWithout || second == TermKind::kWithout) {
        return TermConflict::kExcluded;
    }
    return TermConflict::kNone;
}
}  // namespace rtype::ecs
