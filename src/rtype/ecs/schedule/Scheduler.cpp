/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Scheduler
*/

#include "Scheduler.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtype/ecs/exceptions/ScheduleException.hpp"
#include "rtype/ecs/schedule/DependencyGraph.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
namespace {
/// @return The phase's name, for messages.
[[nodiscard]] std::string_view toString(Phase phase) noexcept {
    switch (phase) {
        case Phase::kStartup:
            return "Startup";
        case Phase::kPreUpdate:
            return "PreUpdate";
        case Phase::kFixedUpdate:
            return "FixedUpdate";
        case Phase::kUpdate:
            return "Update";
        case Phase::kPostUpdate:
            return "PostUpdate";
        case Phase::kRender:
            return "Render";
    }
    return "?";
}

/// @return The cycle's systems as `'a' -> 'b' -> 'a'`.
/// @param cycle Numbers of the systems, the first repeated at the end (DependencyGraph::findCycle()).
/// @param names The system of each number.
[[nodiscard]] std::string describeCycle(std::span<const std::size_t> cycle, std::span<const std::string_view> names) {
    std::string description;
    for (const std::size_t number : cycle) {
        description += std::format("{}'{}'", description.empty() ? "" : " -> ",
                                   *std::next(names.begin(), static_cast<std::ptrdiff_t>(number)));
    }
    return description;
}
}  // namespace

Scheduler::SystemConfig::SystemConfig(Scheduler& scheduler, std::size_t index) noexcept
    : _scheduler{&scheduler}, _index{index} {}

Scheduler::SystemConfig& Scheduler::SystemConfig::after(std::string name) {
    _scheduler->_entries.at(_index).after.push_back(std::move(name));
    _scheduler->_built = false;
    return *this;
}

Scheduler::SystemConfig& Scheduler::SystemConfig::before(std::string name) {
    _scheduler->_entries.at(_index).before.push_back(std::move(name));
    _scheduler->_built = false;
    return *this;
}

Scheduler::SystemConfig& Scheduler::SystemConfig::setSide(Side side) noexcept {
    std::next(_scheduler->_entries.begin(), static_cast<std::ptrdiff_t>(_index))->system.setSide(side);
    return *this;
}

Scheduler::Scheduler(World& world, RunMode mode) noexcept : _world{&world}, _mode{mode} {}

RunMode Scheduler::getRunMode() const noexcept { return _mode; }

std::vector<std::string_view> Scheduler::getOrder(Phase phase) const {
    std::vector<std::string_view> names;
    for (const std::size_t index : _order.at(static_cast<std::size_t>(phase))) {
        names.push_back(_entries.at(index).system.getName());
    }
    return names;
}

Scheduler::SystemConfig Scheduler::addSystem(Phase phase, System system) {
    _entries.push_back(Entry{.system = std::move(system), .phase = phase, .after = {}, .before = {}});
    _built = false;
    return SystemConfig{*this, _entries.size() - 1};
}

void Scheduler::build() {
    _built = false;
    const NameIndex names = indexNames();
    for (std::size_t phase = 0; phase < kPhaseCount; ++phase) {
        _order.at(phase) = sortPhase(static_cast<Phase>(phase), names);
    }
    _built = true;
}

void Scheduler::run(Phase phase) {
    if (!_built) {
        build();
    }
    const std::vector<std::size_t>& order = _order.at(static_cast<std::size_t>(phase));
    for (const std::size_t index : order) {
        System& system = _entries.at(index).system;
        if (runsIn(system.getSide(), _mode)) {
            system.run();
        }
    }
    for (const std::size_t index : order) {  // after the phase: no system of it sees another's structural changes
        System& system = _entries.at(index).system;
        if (runsIn(system.getSide(), _mode)) {
            system.flushCommands();
        }
    }
}

Scheduler::NameIndex Scheduler::indexNames() const {
    NameIndex names;
    for (std::size_t index = 0; index < _entries.size(); ++index) {
        const std::string_view name = _entries.at(index).system.getName();
        if (!names.emplace(name, index).second) {
            throw exceptions::ScheduleException{std::format("Two systems are named '{}': names must be unique", name)};
        }
    }
    return names;
}

std::vector<std::size_t> Scheduler::sortPhase(Phase phase, const NameIndex& names) const {
    const PhaseSystems systems = collect(phase);
    const detail::DependencyGraph graph = makeGraph(phase, systems, names);
    std::optional<std::vector<std::size_t>> order = graph.sort(systems.names);
    if (!order) {
        throw exceptions::ScheduleException{std::format("The before / after constraints of phase {} form a cycle: {}",
                                                        toString(phase),
                                                        describeCycle(graph.findCycle(), systems.names))};
    }
    std::ranges::transform(*order, order->begin(), [&](std::size_t number) { return systems.positions.at(number); });
    return std::move(*order);
}

Scheduler::PhaseSystems Scheduler::collect(Phase phase) const {
    PhaseSystems systems{
        .positions = {}, .names = {}, .numbers = std::vector<std::size_t>(_entries.size(), kNotInPhase)};
    for (std::size_t position = 0; position < _entries.size(); ++position) {
        const Entry& entry = _entries.at(position);
        if (entry.phase == phase) {
            systems.numbers.at(position) = systems.positions.size();
            systems.positions.push_back(position);
            systems.names.push_back(entry.system.getName());
        }
    }
    return systems;
}

detail::DependencyGraph Scheduler::makeGraph(Phase phase, const PhaseSystems& systems, const NameIndex& names) const {
    detail::DependencyGraph graph{systems.positions.size()};
    for (std::size_t number = 0; number < systems.positions.size(); ++number) {
        const Entry& entry = _entries.at(systems.positions.at(number));
        for (const std::string& target : entry.after) {
            graph.addEdge(resolve(phase, systems, names, entry, "after", target), number);
        }
        for (const std::string& target : entry.before) {
            graph.addEdge(number, resolve(phase, systems, names, entry, "before", target));
        }
    }
    return graph;
}

std::size_t Scheduler::resolve(Phase phase, const PhaseSystems& systems, const NameIndex& names, const Entry& entry,
                               std::string_view relation, const std::string& target) const {
    const auto found = names.find(target);
    if (found == names.end()) {
        throw exceptions::ScheduleException{
            std::format("System '{}' runs {} '{}', which is not a system", entry.system.getName(), relation, target)};
    }
    const Phase targetPhase = _entries.at(found->second).phase;
    if (targetPhase != phase) {
        throw exceptions::ScheduleException{
            std::format("System '{}' runs {} '{}', which is in phase {}, not {}: only systems of one phase can be "
                        "ordered",
                        entry.system.getName(), relation, target, toString(targetPhase), toString(phase))};
    }
    return systems.numbers.at(found->second);
}
}  // namespace rtype::ecs
