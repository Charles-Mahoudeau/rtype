/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryMatcher
*/

#include "QueryMatcher.hpp"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <span>

#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/QueryTerm.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"

namespace rtype::ecs::detail {
std::size_t findDriver(std::span<const QueryTerm> terms, std::span<storage::SparseSetStorage* const> pools) noexcept {
    std::size_t driver = 0;
    std::size_t driverSize = std::numeric_limits<std::size_t>::max();
    std::size_t index = 0;
    auto pool = pools.begin();
    for (const QueryTerm& term : terms) {
        if (isRequiredTerm(term.kind) && (*pool)->getSize() < driverSize) {
            driver = index;
            driverSize = (*pool)->getSize();
        }
        ++index;
        ++pool;
    }
    return driver;
}

bool matchEntity(std::span<const QueryTerm> terms, Entity entity, std::span<storage::SparseSetStorage* const> pools,
                 std::span<std::uint32_t> positions, Tick lastRun, Tick thisRun) noexcept {
    auto pool = pools.begin();
    auto position = positions.begin();
    for (const QueryTerm& term : terms) {
        const storage::SparseSetStorage& storage = **pool;
        const std::optional<std::uint32_t> found = storage.find(entity);
        *position = found.value_or(kNoPosition);
        if (term.kind == TermKind::kWithout) {
            if (found) {
                return false;
            }
        } else if (isRequiredTerm(term.kind)) {
            if (!found) {
                return false;
            }
            if (term.kind == TermKind::kAdded &&
                !isNewerThan(*std::next(storage.getAddedTicks().begin(), *found), lastRun, thisRun)) {
                return false;
            }
            if (term.kind == TermKind::kChanged &&
                !isNewerThan(*std::next(storage.getChangedTicks().begin(), *found), lastRun, thisRun)) {
                return false;
            }
        }
        ++pool;
        ++position;
    }
    return true;
}
}  // namespace rtype::ecs::detail
