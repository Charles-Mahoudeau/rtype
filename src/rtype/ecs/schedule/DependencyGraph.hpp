/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DependencyGraph
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "rtype/ecs/Export.hpp"

namespace rtype::ecs::detail {
/// @brief "Runs before" constraints between numbered nodes (the systems of one phase), and the algorithms the
/// scheduler needs on them: a deterministic topological sort and finding a cycle.
/// @code
/// rtype::ecs::detail::DependencyGraph graph{3};
/// graph.addEdge(2, 0);  // node 2 runs before node 0
/// const std::array<std::string_view, 3> names{"damage", "input", "movement"};
/// graph.sort(names);    // {1, 2, 0}: input (smallest name, ready), then movement, then damage
/// @endcode
class RTYPE_ECS_API DependencyGraph {
  public:
    /// @param nodeCount The number of nodes, numbered from 0.
    explicit DependencyGraph(std::size_t nodeCount);
    ~DependencyGraph() = default;
    DependencyGraph(const DependencyGraph& other) = default;
    DependencyGraph& operator=(const DependencyGraph& other) = default;
    DependencyGraph(DependencyGraph&& other) noexcept = default;
    DependencyGraph& operator=(DependencyGraph&& other) noexcept = default;

    /// @return The number of nodes.
    [[nodiscard]] std::size_t getNodeCount() const noexcept;

    /// @brief Records that node `first` runs before node `then`.
    void addEdge(std::size_t first, std::size_t then);

    /// @return Every node, each after the nodes it must follow: a topological sort that always takes, among the
    /// nodes free to go, the one with the smallest name, so the order depends only on the names and the edges.
    /// Nothing if the edges form a cycle.
    /// @param names One name per node, all different.
    [[nodiscard]] std::optional<std::vector<std::size_t>> sort(std::span<const std::string_view> names) const;

    /// @return The nodes of one cycle, the first repeated at the end (`{0, 2, 0}`); empty if there is none.
    [[nodiscard]] std::vector<std::size_t> findCycle() const;

  private:
    /// @brief Where the cycle search is with one node.
    enum class Visit : std::uint8_t {
        kNew,     ///< Not reached yet.
        kOnPath,  ///< On the current path: reaching it again closes a cycle.
        kDone,    ///< Fully explored: no cycle through it.
    };

    /// @return The cycle the search from `start` finds, or empty.
    [[nodiscard]] std::vector<std::size_t> findCycleFrom(std::size_t start, std::vector<Visit>& visits) const;

    std::vector<std::vector<std::size_t>> _next;  ///< _next[a]: the nodes that run after node a.
    std::vector<std::size_t> _previousCount;      ///< _previousCount[b]: the number of nodes that run before node b.
};
}  // namespace rtype::ecs::detail
