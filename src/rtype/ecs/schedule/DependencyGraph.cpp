/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DependencyGraph
*/

#include "DependencyGraph.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <set>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace rtype::ecs::detail {
DependencyGraph::DependencyGraph(std::size_t nodeCount) : _next(nodeCount), _previousCount(nodeCount, 0) {}

std::size_t DependencyGraph::getNodeCount() const noexcept { return _next.size(); }

void DependencyGraph::addEdge(std::size_t first, std::size_t then) {
    _next.at(first).push_back(then);
    ++_previousCount.at(then);
}

std::optional<std::vector<std::size_t>> DependencyGraph::sort(std::span<const std::string_view> names) const {
    // Kahn's algorithm: a node is ready when every node before it is placed; the ready ones wait in a set sorted by
    // name, so the smallest name always goes first.
    const auto nameOf = [&](std::size_t node) { return *std::next(names.begin(), static_cast<std::ptrdiff_t>(node)); };
    std::vector<std::size_t> waiting = _previousCount;
    std::set<std::pair<std::string_view, std::size_t>> ready;
    for (std::size_t node = 0; node < _next.size(); ++node) {
        if (waiting.at(node) == 0) {
            ready.emplace(nameOf(node), node);
        }
    }
    std::vector<std::size_t> order;
    while (!ready.empty()) {
        const std::size_t node = ready.begin()->second;
        ready.erase(ready.begin());
        order.push_back(node);
        for (const std::size_t then : _next.at(node)) {
            if (--waiting.at(then) == 0) {
                ready.emplace(nameOf(then), then);
            }
        }
    }
    if (order.size() != _next.size()) {
        return std::nullopt;  // the nodes left wait on each other
    }
    return order;
}

std::vector<std::size_t> DependencyGraph::findCycle() const {
    std::vector<Visit> visits(_next.size(), Visit::kNew);
    for (std::size_t start = 0; start < _next.size(); ++start) {
        if (visits.at(start) == Visit::kNew) {
            if (std::vector<std::size_t> cycle = findCycleFrom(start, visits); !cycle.empty()) {
                return cycle;
            }
        }
    }
    return {};
}

bool DependencyGraph::reaches(std::size_t from, std::size_t to) const {
    std::vector<bool> seen(_next.size(), false);
    std::vector<std::size_t> toVisit{from};  // an explicit stack, as in findCycleFrom()
    while (!toVisit.empty()) {
        const std::size_t node = toVisit.back();
        toVisit.pop_back();
        for (const std::size_t then : _next.at(node)) {
            if (then == to) {
                return true;
            }
            if (!seen.at(then)) {
                seen.at(then) = true;
                toVisit.push_back(then);
            }
        }
    }
    return false;
}

std::vector<std::size_t> DependencyGraph::findCycleFrom(std::size_t start, std::vector<Visit>& visits) const {
    // A depth-first search with an explicit stack, so a long chain of constraints cannot overflow the call stack.
    std::vector<std::pair<std::size_t, std::size_t>> path{{start, 0}};  // {node, next edge to follow}
    visits.at(start) = Visit::kOnPath;
    while (!path.empty()) {
        const std::size_t node = path.back().first;
        const std::size_t edge = path.back().second;
        if (edge == _next.at(node).size()) {
            visits.at(node) = Visit::kDone;
            path.pop_back();
            continue;
        }
        ++path.back().second;
        const std::size_t to = _next.at(node).at(edge);
        if (visits.at(to) == Visit::kOnPath) {  // back to a node of the path: the path from it is a cycle
            std::vector<std::size_t> cycle;
            const auto first = std::ranges::find(path, to, &std::pair<std::size_t, std::size_t>::first);
            for (auto step = first; step != path.end(); ++step) {
                cycle.push_back(step->first);
            }
            cycle.push_back(to);
            return cycle;
        }
        if (visits.at(to) == Visit::kNew) {
            visits.at(to) = Visit::kOnPath;
            path.emplace_back(to, 0);
        }
    }
    return {};
}
}  // namespace rtype::ecs::detail
