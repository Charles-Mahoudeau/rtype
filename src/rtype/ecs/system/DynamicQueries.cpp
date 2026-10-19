/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DynamicQueries
*/

#include "DynamicQueries.hpp"

#include <cassert>
#include <cstddef>
#include <format>
#include <iterator>
#include <span>
#include <utility>
#include <vector>

#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/exceptions/QueryException.hpp"
#include "rtype/ecs/query/DynamicQuery.hpp"
#include "rtype/ecs/query/QueryIteration.hpp"
#include "rtype/ecs/query/QueryTerm.hpp"
#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/system/SystemParam.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
DynamicQueries::DynamicQueries(std::span<const DynamicQuery> queries, Tick lastRun) noexcept
    : _queries{queries}, _lastRun{lastRun} {}

std::size_t DynamicQueries::getCount() const noexcept { return _queries.size(); }

const DynamicQuery& DynamicQueries::getQuery(std::size_t index) const noexcept {
    assert(index < _queries.size());
    return *std::next(_queries.begin(), static_cast<std::ptrdiff_t>(index));
}

Tick DynamicQueries::getLastRun() const noexcept { return _lastRun; }

QueryIteration DynamicQueries::iter(std::size_t index) const { return getQuery(index).iter(_lastRun); }

QueryIteration DynamicQueries::iterSince(std::size_t index, Tick since) const { return getQuery(index).iter(since); }

SystemParam<DynamicQueries>::State SystemParam<DynamicQueries>::init(World& world, Config config) {
    State queries;
    queries.reserve(config.queries.size());
    for (std::size_t index = 0; index < config.queries.size(); ++index) {
        Result<DynamicQuery> query = config.queries.at(index).build(world.getComponents());
        if (!query) {
            throw exceptions::QueryException{
                std::format("Query {} of the system cannot be built: {}", index, query.error().getMessage())};
        }
        queries.push_back(std::move(*query));
    }
    return queries;
}

DynamicQueries SystemParam<DynamicQueries>::get(State& state, const SystemContext& context) noexcept {
    return DynamicQueries{state, context.lastRun};
}

void SystemParam<DynamicQueries>::addAccess(const State& state, SystemAccess& access) {
    for (const DynamicQuery& query : state) {
        for (const ComponentAccess& component : query.getAccess()) {
            access.addComponent(component.id, component.write);
        }
    }
}
}  // namespace rtype::ecs
