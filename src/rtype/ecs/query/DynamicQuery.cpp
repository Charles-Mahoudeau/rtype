/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DynamicQuery
*/

#include "DynamicQuery.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <iterator>
#include <span>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>
#include <vector>

#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/query/QueryIteration.hpp"
#include "rtype/ecs/query/QueryTerm.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"

namespace rtype::ecs {
namespace {
/// @return The name of a registered component, for error messages.
[[nodiscard]] std::string_view nameOf(const Components& components, ComponentId id) noexcept {
    return components.getRegistry().findById(id)->type.name;
}

/// @return Why two terms on the same component cannot be in one query, or nothing if they can.
[[nodiscard]] Result<void> checkPair(std::string_view name, TermKind first, TermKind second) {
    switch (findConflict(first, second)) {
        case TermConflict::kNone:
            return {};
        case TermConflict::kSameTerm:
            return fail(ErrorKind::kInvalidQuery, std::format("Query: component '{}' has the same term twice", name));
        case TermConflict::kTwoDataTerms:
            return fail(ErrorKind::kInvalidQuery, std::format("Query: component '{}' is in two data terms", name));
        case TermConflict::kExcluded:
            return fail(ErrorKind::kInvalidQuery,
                        std::format("Query: component '{}' is excluded by without() and used by another term", name));
    }
    return {};
}

/// @return Every problem create() rejects, the first one found.
[[nodiscard]] Result<void> validate(const Components& components, std::span<const QueryTerm> terms) {
    for (const QueryTerm& term : terms) {
        if (components.findPool(term.id) == nullptr) {
            return fail(ErrorKind::kInvalidQuery,
                        std::format("Query: component id {} is not registered in this world", term.id));
        }
    }
    if (std::ranges::none_of(terms, [](const QueryTerm& term) { return isRequiredTerm(term.kind); })) {
        return fail(ErrorKind::kInvalidQuery,
                    "Query: it needs at least one required term (read, write, with, added or changed)");
    }
    for (auto first = terms.begin(); first != terms.end(); ++first) {
        for (auto second = std::next(first); second != terms.end(); ++second) {
            if (first->id != second->id) {
                continue;
            }
            if (Result<void> pair = checkPair(nameOf(components, first->id), first->kind, second->kind); !pair) {
                return pair;
            }
        }
    }
    return {};
}
}  // namespace

Result<DynamicQuery> DynamicQuery::create(Components& components, std::vector<QueryTerm> terms) {
    if (Result<void> valid = validate(components, terms); !valid) {
        return tl::unexpected<Error>{std::move(valid.error())};
    }
    return DynamicQuery{components, std::move(terms)};
}

DynamicQuery::DynamicQuery(Components& components, std::vector<QueryTerm> terms)
    : _components{&components}, _terms{std::move(terms)} {
    std::size_t index = 0;
    for (const QueryTerm& term : _terms) {
        if (isDataTerm(term.kind)) {
            _dataTerms.push_back(index);
        }
        ++index;
        if (!isDataTerm(term.kind) && term.kind != TermKind::kAdded && term.kind != TermKind::kChanged) {
            continue;  // with and without access no value
        }
        const bool write = isWriteTerm(term.kind);
        const auto known = std::ranges::find(_access, term.id, &ComponentAccess::id);
        if (known != _access.end()) {
            known->write = known->write || write;
        } else {
            _access.push_back(ComponentAccess{.id = term.id, .write = write});
        }
    }
}

std::span<const QueryTerm> DynamicQuery::getTerms() const noexcept { return _terms; }

std::size_t DynamicQuery::getDataTermCount() const noexcept { return _dataTerms.size(); }

std::span<const std::size_t> DynamicQuery::getDataTerms() const noexcept { return _dataTerms; }

std::span<const ComponentAccess> DynamicQuery::getAccess() const noexcept { return _access; }

QueryIteration DynamicQuery::iter(Tick lastRun) const {
    return QueryIteration{*this, lastRun, _components->getChangeTick()};
}

void DynamicQuery::resolvePools(std::span<storage::SparseSetStorage*> pools) const noexcept {
    auto pool = pools.begin();
    for (const QueryTerm& term : _terms) {
        *pool = _components->findPool(term.id);  // never null: create() checked every id, and pools are never removed
        ++pool;
    }
}
}  // namespace rtype::ecs
