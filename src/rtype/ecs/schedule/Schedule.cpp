/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Schedule
*/

#include "Schedule.hpp"

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

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/event/EventInfo.hpp"
#include "rtype/ecs/exceptions/ScheduleException.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
#include "rtype/ecs/schedule/DependencyGraph.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
namespace {
/// @return The first type both lists use, one of them writing it, apart from those of `allowed`; nothing if there is
/// none.
[[nodiscard]] std::optional<TypeId> findSharedWrite(std::span<const TypeAccess> first,
                                                    std::span<const TypeAccess> second,
                                                    std::span<const TypeId> allowed = {}) {
    for (const TypeAccess& access : first) {
        if (std::ranges::find(allowed, access.id) != allowed.end()) {
            continue;
        }
        const auto other = std::ranges::find(second, access.id, &TypeAccess::id);
        if (other != second.end() && (access.write || other->write)) {
            return access.id;
        }
    }
    return std::nullopt;
}

/// @return The numbers 0..n-1 of `names`, sorted by name.
[[nodiscard]] std::vector<std::size_t> sortByName(std::span<const std::string_view> names) {
    std::vector<std::size_t> numbers;
    numbers.reserve(names.size());
    for (std::size_t number = 0; number < names.size(); ++number) {
        numbers.push_back(number);
    }
    std::ranges::sort(numbers, {}, [&](std::size_t number) {
        return *std::next(names.begin(), static_cast<std::ptrdiff_t>(number));
    });
    return numbers;
}

/// @return Each line on its own indented line, for a message listing problems.
[[nodiscard]] std::string joinLines(std::span<const std::string> lines) {
    std::string joined;
    for (const std::string& line : lines) {
        joined += std::format("\n  {}", line);
    }
    return joined;
}

/// @return The cycle's systems and sets as `'a' -> set 'effects' -> 'a'`.
/// @param cycle Numbers of the nodes, the first repeated at the end (DependencyGraph::findCycle()).
/// @param shown Each node as messages show it.
/// @param systemCount The number of systems: the nodes after them are sets, a start then an end each, shown once when
/// the cycle crosses a set from its start to its end.
[[nodiscard]] std::string describeCycle(std::span<const std::size_t> cycle, std::span<const std::string> shown,
                                        std::size_t systemCount) {
    std::string description;
    std::optional<std::size_t> previous;
    for (const std::size_t number : cycle) {
        const bool setEnd = number >= systemCount && (number - systemCount) % 2 == 1;
        if (!(setEnd && previous == number - 1)) {
            description += std::format("{}{}", description.empty() ? "" : " -> ",
                                       *std::next(shown.begin(), static_cast<std::ptrdiff_t>(number)));
        }
        previous = number;
    }
    return description;
}
}  // namespace

Schedule::SystemConfig::SystemConfig(Schedule& schedule, std::size_t index) noexcept
    : _schedule{&schedule}, _index{index} {}

Schedule::SystemConfig& Schedule::SystemConfig::after(std::string name) {
    _schedule->_entries.at(_index).after.push_back(std::move(name));
    _schedule->_built = false;
    return *this;
}

Schedule::SystemConfig& Schedule::SystemConfig::before(std::string name) {
    _schedule->_entries.at(_index).before.push_back(std::move(name));
    _schedule->_built = false;
    return *this;
}

Schedule::SystemConfig& Schedule::SystemConfig::inSet(std::string set) {
    static_cast<void>(_schedule->_sets.try_emplace(set));
    _schedule->_entries.at(_index).sets.push_back(std::move(set));
    _schedule->_built = false;
    return *this;
}

Schedule::SystemConfig& Schedule::SystemConfig::setSide(Side side) {
    System& system = std::next(_schedule->_entries.begin(), static_cast<std::ptrdiff_t>(_index))->system;
    if (side == Side::kClient) {
        _schedule->checkClientWrites(system);
    }
    system.setSide(side);
    _schedule->_built = false;  // the side decides which events the schedule writes in this mode
    return *this;
}

void Schedule::checkClientWrites(const System& system) const {
    const ComponentRegistry& registry = _world->getComponents().getRegistry();
    for (const TypeAccess& access : system.getAccess().getComponents()) {
        const ComponentInfo* info = registry.findById(access.id);
        if (!access.write || info == nullptr) {
            continue;
        }
        const bool replicated = info->type.replication != ReplicationMode::kNone;
        if (replicated || info->type.presence == Presence::kServerOnly) {
            throw exceptions::ScheduleException{std::format(
                "System '{}' of phase {} runs on clients but writes component '{}', which is {}", system.getName(),
                toString(_phase), info->type.name,
                replicated ? "replicated: the server would overwrite it" : "server-only: clients do not have it")};
        }
    }
}

Schedule::SetConfig::SetConfig(Schedule& schedule, std::string name) : _schedule{&schedule}, _name{std::move(name)} {}

Schedule::SetConfig& Schedule::SetConfig::after(std::string name) {
    _schedule->_sets.at(_name).after.push_back(std::move(name));
    _schedule->_built = false;
    return *this;
}

Schedule::SetConfig& Schedule::SetConfig::before(std::string name) {
    _schedule->_sets.at(_name).before.push_back(std::move(name));
    _schedule->_built = false;
    return *this;
}

Schedule::Schedule(World& world, RunMode mode, Phase phase) noexcept : _world{&world}, _mode{mode}, _phase{phase} {}

RunMode Schedule::getRunMode() const noexcept { return _mode; }

Phase Schedule::getPhase() const noexcept { return _phase; }

std::vector<std::string_view> Schedule::getOrder() const {
    std::vector<std::string_view> names;
    names.reserve(_order.size());
    for (const std::size_t index : _order) {
        names.push_back(_entries.at(index).system.getName());
    }
    return names;
}

std::span<const EventId> Schedule::getWrittenEvents() const noexcept { return _writtenEvents; }

bool Schedule::isBuilt() const noexcept { return _built; }

bool Schedule::contains(std::string_view name) const noexcept {
    return std::ranges::any_of(_entries, [&](const Entry& entry) { return entry.system.getName() == name; });
}

const System* Schedule::findSystem(std::string_view name) const noexcept {
    const auto found =
        std::ranges::find_if(_entries, [&](const Entry& entry) { return entry.system.getName() == name; });
    return found == _entries.end() ? nullptr : &found->system;
}

bool Schedule::containsSet(std::string_view name) const noexcept { return _sets.contains(name); }

bool Schedule::isAmbiguousResource(ResourceId id) const noexcept {
    return std::ranges::find(_ambiguousResources, id) != _ambiguousResources.end();
}

void Schedule::allowAmbiguousResource(ResourceId id) {
    if (!isAmbiguousResource(id)) {
        _ambiguousResources.push_back(id);
    }
    _built = false;
}

Schedule::SetConfig Schedule::configureSet(std::string name) {
    static_cast<void>(_sets.try_emplace(name));
    _built = false;
    return SetConfig{*this, std::move(name)};
}

Schedule::SystemConfig Schedule::addSystem(System system) {
    if (contains(system.getName())) {
        throw exceptions::ScheduleException{std::format("Two systems of phase {} are named '{}': names must be unique",
                                                        toString(_phase), system.getName())};
    }
    _entries.push_back(Entry{.system = std::move(system), .after = {}, .before = {}, .sets = {}});
    _built = false;
    return SystemConfig{*this, _entries.size() - 1};
}

void Schedule::build() {
    _built = false;
    checkSetNames();
    const Graph graph = makeGraph();
    const std::vector<std::string_view> nodeNames{graph.names.begin(), graph.names.end()};
    std::optional<std::vector<std::size_t>> order = graph.graph.sort(nodeNames);
    if (!order) {
        throw exceptions::ScheduleException{
            std::format("The before / after constraints of phase {} form a cycle: {}", toString(_phase),
                        describeCycle(graph.graph.findCycle(), graph.shown, _entries.size()))};
    }
    checkAmbiguities(getNames(), graph.graph);
    std::erase_if(*order, [&](std::size_t node) { return node >= _entries.size(); });  // a set's nodes do not run
    _order = std::move(*order);
    _writtenEvents = collectWrittenEvents();
    _built = true;
}

void Schedule::run() {
    if (!_built) {
        build();
    }
    for (const std::size_t index : _order) {
        System& system = _entries.at(index).system;
        if (runsIn(system.getSide(), _mode)) {
            system.run();
        }
    }
    for (const std::size_t index : _order) {  // after the phase: no system of it sees another's structural changes
        System& system = _entries.at(index).system;
        if (runsIn(system.getSide(), _mode)) {
            system.flushCommands();
        }
    }
}

std::vector<std::string_view> Schedule::getNames() const {
    std::vector<std::string_view> names;
    names.reserve(_entries.size());
    for (const Entry& entry : _entries) {
        names.push_back(entry.system.getName());
    }
    return names;
}

Schedule::Graph Schedule::makeGraph() const {
    NameIndex names;
    for (std::size_t position = 0; position < _entries.size(); ++position) {
        names.emplace(_entries.at(position).system.getName(), position);
    }
    Graph graph{.graph = detail::DependencyGraph{_entries.size() + (2 * _sets.size())}, .names = {}, .shown = {}};
    for (const Entry& entry : _entries) {
        graph.names.emplace_back(entry.system.getName());
        graph.shown.push_back(std::format("'{}'", entry.system.getName()));
    }
    for (const auto& [name, set] : _sets) {
        // The control character keeps a set's nodes from sharing a name with a system in the sort.
        graph.names.push_back(std::format("\x1F{}:start", name));
        graph.names.push_back(std::format("\x1F{}:end", name));
        graph.shown.push_back(std::format("set '{}'", name));
        graph.shown.push_back(std::format("set '{}'", name));
    }
    for (const auto& [name, set] : _sets) {
        const std::size_t start = getSetStart(name);
        const std::string owner = std::format("Set '{}'", name);
        graph.graph.addEdge(start, start + 1);  // keeps the order through an empty set
        for (const std::string& target : set.after) {
            graph.graph.addEdge(resolve(names, owner, "after", target), start);
        }
        for (const std::string& target : set.before) {
            graph.graph.addEdge(start + 1, resolve(names, owner, "before", target));
        }
    }
    for (std::size_t position = 0; position < _entries.size(); ++position) {
        const Entry& entry = _entries.at(position);
        const std::string owner = std::format("System '{}'", entry.system.getName());
        for (const std::string& set : entry.sets) {
            const std::size_t start = getSetStart(set);
            graph.graph.addEdge(start, position);
            graph.graph.addEdge(position, start + 1);
        }
        for (const std::string& target : entry.after) {
            graph.graph.addEdge(resolve(names, owner, "after", target), position);
        }
        for (const std::string& target : entry.before) {
            graph.graph.addEdge(position, resolve(names, owner, "before", target));
        }
    }
    return graph;
}

std::size_t Schedule::resolve(const NameIndex& names, std::string_view owner, std::string_view relation,
                              const std::string& target) const {
    if (const auto system = names.find(target); system != names.end()) {
        return system->second;
    }
    if (_sets.contains(target)) {
        const std::size_t start = getSetStart(target);
        return relation == "after" ? start + 1 : start;  // after a set: after its end; before it: before its start
    }
    throw exceptions::ScheduleException{
        std::format("{} runs {} '{}', which is neither a system nor a set of phase {}: only systems and sets of one "
                    "phase can be ordered",
                    owner, relation, target, toString(_phase))};
}

std::size_t Schedule::getSetStart(std::string_view name) const {
    const auto set = _sets.find(name);
    return _entries.size() + (2 * static_cast<std::size_t>(std::distance(_sets.begin(), set)));
}

void Schedule::checkSetNames() const {
    for (const auto& [name, set] : _sets) {
        if (contains(name)) {
            throw exceptions::ScheduleException{
                std::format("Phase {} has a system and a set both named '{}': systems and sets share their names",
                            toString(_phase), name)};
        }
    }
}

void Schedule::checkAmbiguities(const std::vector<std::string_view>& names,
                                const detail::DependencyGraph& graph) const {
    const std::vector<std::string> ambiguities = findAmbiguities(names, graph);
    if (!ambiguities.empty()) {
        throw exceptions::ScheduleException{
            std::format("Systems of phase {} touch the same data with no order between them, so their result would "
                        "depend on the order they were added; order them with before / after:{}",
                        toString(_phase), joinLines(ambiguities))};
    }
}

std::vector<std::string> Schedule::findAmbiguities(const std::vector<std::string_view>& names,
                                                   const detail::DependencyGraph& graph) const {
    // Pairs in name order, so the lines never depend on the order systems were added.
    const std::vector<std::size_t> byName = sortByName(names);
    std::vector<std::string> ambiguities;
    for (auto first = byName.begin(); first != byName.end(); ++first) {
        for (auto second = std::next(first); second != byName.end(); ++second) {
            if (graph.reaches(*first, *second) || graph.reaches(*second, *first)) {
                continue;  // ordered: they always run in the same order
            }
            const std::optional<std::string> shared =
                findConflict(_entries.at(*first).system, _entries.at(*second).system);
            if (shared) {
                ambiguities.push_back(std::format("'{}' and '{}' both use {}, one of them writing it", names.at(*first),
                                                  names.at(*second), *shared));
            }
        }
    }
    return ambiguities;
}

std::optional<std::string> Schedule::findConflict(const System& first, const System& second) const {
    const SystemAccess& a = first.getAccess();
    const SystemAccess& b = second.getAccess();
    if (const std::optional<TypeId> id = findSharedWrite(a.getComponents(), b.getComponents())) {
        return std::format("component '{}'", _world->getComponents().getRegistry().findById(*id)->type.name);
    }
    if (const std::optional<TypeId> id = findSharedWrite(a.getResources(), b.getResources(), _ambiguousResources)) {
        return std::format("resource '{}'", _world->getResources().getRegistry().findById(*id)->type.name);
    }
    if (const std::optional<TypeId> id = findSharedWrite(a.getEvents(), b.getEvents())) {
        return std::format("event '{}'", _world->getEvents().getRegistry().findById(*id)->type.name);
    }
    return std::nullopt;
}

std::vector<EventId> Schedule::collectWrittenEvents() const {
    std::vector<EventId> events;
    for (const Entry& entry : _entries) {
        if (!runsIn(entry.system.getSide(), _mode)) {
            continue;
        }
        for (const TypeAccess& access : entry.system.getAccess().getEvents()) {
            if (access.write) {
                events.push_back(access.id);
            }
        }
    }
    std::ranges::sort(events);
    const auto duplicates = std::ranges::unique(events);
    events.erase(duplicates.begin(), duplicates.end());
    return events;
}
}  // namespace rtype::ecs
