/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TermTraits
*/

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

#include "rtype/ecs/query/DynamicQuery.hpp"
#include "rtype/ecs/storage/ViewAs.hpp"

namespace rtype::ecs {
template <Component T>
Mut<T> TermTraits<T>::fetch(storage::SparseSetStorage& pool, std::uint32_t position, Tick thisRun) noexcept {
    return Mut<T>{storage::viewAs<T>(pool.at(position)), &*std::next(pool.getChangedTicks().begin(), position),
                  thisRun};
}

template <Component T>
const T& TermTraits<const T>::fetch(storage::SparseSetStorage& pool, std::uint32_t position,
                                    Tick /*thisRun*/) noexcept {
    return *storage::viewAs<T>(pool.at(position));
}

template <Component T>
Mut<T> TermTraits<Optional<T>>::fetch(storage::SparseSetStorage& pool, std::uint32_t position, Tick thisRun) noexcept {
    if (position == DynamicQuery::kNoPosition) {
        return {};
    }
    return TermTraits<T>::fetch(pool, position, thisRun);
}

template <Component T>
const T* TermTraits<Optional<const T>>::fetch(storage::SparseSetStorage& pool, std::uint32_t position,
                                              Tick /*thisRun*/) noexcept {
    if (position == DynamicQuery::kNoPosition) {
        return nullptr;
    }
    return &TermTraits<const T>::fetch(pool, position, 0);
}

namespace detail {
template <TypedTerm... Terms>
consteval bool areCompatibleTerms() noexcept {
    /// @brief What the checks need of one term: its component's identity, its kind, and whether it reads a tag.
    struct Identity {
        std::uint64_t component;  ///< ComponentTraits::kHash of the component: one per name, like the registry.
        TermKind kind;            ///< What the term asks of it.
        bool dataOnTag;           ///< A data term on an empty type: there is no value to yield.
    };
    const std::array<Identity, sizeof...(Terms)> terms{Identity{
        .component = ComponentTraits<typename TermTraits<Terms>::Type>::kHash,
        .kind = TermTraits<Terms>::kKind,
        .dataOnTag = isDataTerm(TermTraits<Terms>::kKind) && std::is_empty_v<typename TermTraits<Terms>::Type>}...};

    if (std::ranges::none_of(terms, [](const Identity& term) { return isRequiredTerm(term.kind); })) {
        return false;
    }
    if (std::ranges::any_of(terms, &Identity::dataOnTag)) {
        return false;
    }
    for (auto first = terms.begin(); first != terms.end(); ++first) {
        for (auto second = std::next(first); second != terms.end(); ++second) {
            if (first->component == second->component &&
                findConflict(first->kind, second->kind) != TermConflict::kNone) {
                return false;
            }
        }
    }
    return true;
}
}  // namespace detail
}  // namespace rtype::ecs
