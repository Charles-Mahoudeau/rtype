/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DynamicCalls
*/

#include "DynamicCalls.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <format>
#include <iterator>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "rtype/ecs/call/CallInfo.hpp"
#include "rtype/ecs/call/Calls.hpp"
#include "rtype/ecs/exceptions/CallException.hpp"
#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/system/SystemParam.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
DynamicCalls::DynamicCalls(std::span<const CallInfo* const> infos, std::span<detail::CallBody* const> bodies,
                           const SystemContext& context) noexcept
    : _infos{infos}, _bodies{bodies}, _context{context} {}

std::size_t DynamicCalls::getCount() const noexcept { return _infos.size(); }

const CallInfo& DynamicCalls::getInfo(const std::size_t index) const noexcept {
    assert(index < _infos.size());
    return **std::next(_infos.begin(), static_cast<std::ptrdiff_t>(index));
}

void DynamicCalls::invoke(const std::size_t index, const std::span<const std::byte> arguments,
                          const std::span<std::byte> result) const {
    assert(index < _bodies.size());
    checkCallBytes(getInfo(index), arguments, result);
    (*std::next(_bodies.begin(), static_cast<std::ptrdiff_t>(index)))->invoke(_context, arguments, result);
}

SystemParam<DynamicCalls>::State SystemParam<DynamicCalls>::init(World& world, Config config) {
    State state;
    const std::vector<CallId> ids = std::move(config.calls);
    for (const CallId id : ids) {
        const CallInfo* info = world.getCalls().findById(id);
        if (info == nullptr) {
            throw exceptions::CallException{std::format("A call of the system (id {}) is not registered", id)};
        }
        state.infos.push_back(info);
        const auto shared = std::ranges::find(state.infos, info);
        if (shared != std::prev(state.infos.end())) {
            state.bodies.push_back(state.bodies.at(static_cast<std::size_t>(shared - state.infos.begin())));
            continue;
        }
        state.owned.push_back(info->body->bind(world));
        state.bodies.push_back(state.owned.back().get());
    }
    return state;
}

DynamicCalls SystemParam<DynamicCalls>::get(State& state, const SystemContext& context) noexcept {
    return DynamicCalls{state.infos, state.bodies, context};
}

void SystemParam<DynamicCalls>::addAccess(const State& state, SystemAccess& access) {
    for (const CallInfo* info : state.infos) {
        access.merge(info->access);
    }
}
}  // namespace rtype::ecs
