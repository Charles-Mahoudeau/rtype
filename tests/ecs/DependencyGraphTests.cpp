/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DependencyGraphTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

#include "rtype/ecs/schedule/DependencyGraph.hpp"

namespace {
using rtype::ecs::detail::DependencyGraph;
using Order = std::vector<std::size_t>;
}  // namespace

TEST(DependencyGraph, SortsUnconstrainedNodesByName) {
    const DependencyGraph graph{3};
    const std::array<std::string_view, 3> names{"c", "a", "b"};
    EXPECT_EQ(graph.sort(names), std::optional<Order>{(Order{1, 2, 0})});
}

TEST(DependencyGraph, PlacesEachNodeAfterTheNodesBeforeIt) {
    DependencyGraph graph{4};
    graph.addEdge(3, 0);  // d before a
    graph.addEdge(0, 1);  // a before b
    const std::array<std::string_view, 4> names{"a", "b", "c", "d"};
    EXPECT_EQ(graph.sort(names), std::optional<Order>{(Order{2, 3, 0, 1})});  // c and d free: c first by name
}

TEST(DependencyGraph, AcceptsTheSameEdgeTwice) {
    DependencyGraph graph{2};
    graph.addEdge(0, 1);
    graph.addEdge(0, 1);  // the same constraint written twice (after on one side, before on the other)
    const std::array<std::string_view, 2> names{"b", "a"};
    EXPECT_EQ(graph.sort(names), std::optional<Order>{(Order{0, 1})});
}

TEST(DependencyGraph, FindsNoOrderAndTheCycleWhenNodesWaitOnEachOther) {
    DependencyGraph graph{4};
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    graph.addEdge(2, 0);
    graph.addEdge(2, 3);  // 3 waits on the cycle without being part of it
    const std::array<std::string_view, 4> names{"a", "b", "c", "d"};
    EXPECT_FALSE(graph.sort(names));
    EXPECT_EQ(graph.findCycle(), (Order{0, 1, 2, 0}));
}

TEST(DependencyGraph, FindsASelfLoop) {
    DependencyGraph graph{2};
    graph.addEdge(1, 1);
    EXPECT_EQ(graph.findCycle(), (Order{1, 1}));
}

TEST(DependencyGraph, FindsNoCycleInAnOrderableGraph) {
    DependencyGraph graph{3};
    graph.addEdge(0, 2);
    graph.addEdge(1, 2);  // two paths into 2: a diamond is not a cycle
    EXPECT_TRUE(graph.findCycle().empty());
    EXPECT_EQ(graph.getNodeCount(), 3U);
}

TEST(DependencyGraph, HandlesALongChainWithoutRecursion) {
    constexpr std::size_t kLength = 100'000;
    DependencyGraph graph{kLength};
    for (std::size_t node = 0; node + 1 < kLength; ++node) {
        graph.addEdge(node, node + 1);
    }
    graph.addEdge(kLength - 1, 0);  // closes one long cycle
    EXPECT_EQ(graph.findCycle().size(), kLength + 1);
}

TEST(DependencyGraph, TellsWhetherANodeReachesAnother) {
    DependencyGraph graph{4};
    graph.addEdge(0, 1);
    graph.addEdge(1, 2);
    EXPECT_TRUE(graph.reaches(0, 1));
    EXPECT_TRUE(graph.reaches(0, 2));   // through 1
    EXPECT_FALSE(graph.reaches(2, 0));  // edges have a direction
    EXPECT_FALSE(graph.reaches(0, 3));  // 3 is not linked
    EXPECT_FALSE(graph.reaches(1, 1));  // not on a cycle
}

TEST(DependencyGraph, ANodeOnACycleReachesItself) {
    DependencyGraph graph{2};
    graph.addEdge(0, 1);
    graph.addEdge(1, 0);
    EXPECT_TRUE(graph.reaches(0, 0));
}
