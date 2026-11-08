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
/// @return The first type both lists use, one of them writing it and not both only appending to it; nothing if there
/// is none.
[[nodiscard]] std::optional<TypeId> findSharedWrite(std::span<const TypeAccess> first,
                                                    std::span<const TypeAccess> second) {
    for (const TypeAccess& access : first) {
        const auto other = std::ranges::find(second, access.id, &TypeAccess::id);
        if (other != second.end() && (access.write || other->write) && !(access.append && other->append)) {
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

Schedule::SystemConfig Schedule::addSystem(System system) {
    if (contains(system.getName())) {
        throw exceptions::ScheduleException{std::format("Two systems of phase {} are named '{}': names must be unique",
                                                        toString(_phase), system.getName())};
    }
    _entries.push_back(Entry{.system = std::move(system), .after = {}, .before = {}});
    _built = false;
    return SystemConfig{*this, _entries.size() - 1};
}

void Schedule::replaceSystems(std::vector<SystemSpec> systems, std::span<const std::string> removed) {
    // Everything that can be refused is refused before the schedule changes.
    for (const std::string& name : removed) {
        if (!contains(name)) {
            throw exceptions::ScheduleException{
                std::format("Cannot remove system '{}': it is not a system of phase {}", name, toString(_phase))};
        }
    }
    for (auto spec = systems.begin(); spec != systems.end(); ++spec) {
        const std::string_view name = spec->system.getName();
        if (std::ranges::find(removed, name) != removed.end() ||
            std::any_of(systems.begin(), spec,
                        [name](const SystemSpec& other) { return other.system.getName() == name; })) {
            throw exceptions::ScheduleException{std::format(
                "System '{}' of phase {} is put in the schedule twice, or removed and put", name, toString(_phase))};
        }
        if (spec->system.getSide() == Side::kClient) {
            checkClientWrites(spec->system);
        }
    }

    // The old state, kept to put back if the new schedule does not build.
    std::vector<Entry> old = std::move(_entries);
    std::vector<std::string> oldNames;
    oldNames.reserve(old.size());
    for (const Entry& entry : old) {
        oldNames.emplace_back(entry.system.getName());
    }
    std::vector<bool> carried(old.size(), false);  // moved to _entries: the entry of `old` is hollow
    const std::vector<std::size_t> oldOrder = _order;
    const std::vector<EventId> oldEvents = _writtenEvents;
    const bool wasBuilt = _built;
    _entries.clear();

    std::vector<bool> used(systems.size(), false);
    for (std::size_t position = 0; position < old.size(); ++position) {
        const std::string& name = oldNames.at(position);
        if (std::ranges::find(removed, name) != removed.end()) {
            continue;
        }
        const auto replacement =
            std::ranges::find_if(systems, [&name](const SystemSpec& spec) { return spec.system.getName() == name; });
        if (replacement != systems.end()) {
            used.at(static_cast<std::size_t>(replacement - systems.begin())) = true;
            _entries.push_back(Entry{
                .system = std::move(replacement->system),
                .after = std::move(replacement->after),
                .before = std::move(replacement->before),
            });
            continue;
        }
        _entries.push_back(std::move(old.at(position)));
        carried.at(position) = true;
    }
    for (std::size_t index = 0; index < systems.size(); ++index) {
        if (!used.at(index)) {
            SystemSpec& spec = systems.at(index);
            _entries.push_back(Entry{
                .system = std::move(spec.system),
                .after = std::move(spec.after),
                .before = std::move(spec.before),
            });
        }
    }

    try {
        build();
    } catch (...) {
        std::vector<Entry> restored;
        restored.reserve(old.size());
        for (std::size_t position = 0; position < old.size(); ++position) {
            if (!carried.at(position)) {
                restored.push_back(std::move(old.at(position)));
                continue;
            }
            const std::string& name = oldNames.at(position);
            const auto kept =
                std::ranges::find_if(_entries, [&name](const Entry& entry) { return entry.system.getName() == name; });
            restored.push_back(std::move(*kept));
        }
        _entries = std::move(restored);
        _order = oldOrder;
        _writtenEvents = oldEvents;
        _built = wasBuilt;
        throw;
    }
}

void Schedule::build() {
    _built = false;
    const std::vector<std::string_view> names = getNames();
    const detail::DependencyGraph graph = makeGraph();
    std::optional<std::vector<std::size_t>> order = graph.sort(names);
    if (!order) {
        throw exceptions::ScheduleException{std::format("The before / after constraints of phase {} form a cycle: {}",
                                                        toString(_phase), describeCycle(graph.findCycle(), names))};
    }
    checkAmbiguities(names, graph);
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

detail::DependencyGraph Schedule::makeGraph() const {
    NameIndex names;
    for (std::size_t position = 0; position < _entries.size(); ++position) {
        names.emplace(_entries.at(position).system.getName(), position);
    }
    detail::DependencyGraph graph{_entries.size()};
    for (std::size_t position = 0; position < _entries.size(); ++position) {
        const Entry& entry = _entries.at(position);
        for (const std::string& target : entry.after) {
            graph.addEdge(resolve(names, entry, "after", target), position);
        }
        for (const std::string& target : entry.before) {
            graph.addEdge(position, resolve(names, entry, "before", target));
        }
    }
    if (_mode == RunMode::kStandalone) {
        // Both sides share one world: a server system and a client system that touch the same data are not ambiguous,
        // the server's runs first, as the client of a network game sees what the server did. A pair the game ordered
        // itself keeps its order.
        for (std::size_t server = 0; server < _entries.size(); ++server) {
            for (std::size_t client = 0; client < _entries.size(); ++client) {
                const System& first = _entries.at(server).system;
                const System& second = _entries.at(client).system;
                if (first.getSide() == Side::kServer && second.getSide() == Side::kClient &&
                    !graph.reaches(server, client) && !graph.reaches(client, server) && findConflict(first, second)) {
                    graph.addEdge(server, client);
                }
            }
        }
    }
    return graph;
}

std::size_t Schedule::resolve(const NameIndex& names, const Entry& entry, std::string_view relation,
                              const std::string& target) const {
    const auto found = names.find(target);
    if (found == names.end()) {
        throw exceptions::ScheduleException{
            std::format("System '{}' runs {} '{}', which is not a system of phase {}: only systems of one phase can "
                        "be ordered",
                        entry.system.getName(), relation, target, toString(_phase))};
    }
    return found->second;
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
    if (const std::optional<TypeId> id = findSharedWrite(a.getResources(), b.getResources())) {
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
