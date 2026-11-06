/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UiLayoutTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <string>
#include <vector>

#include "plugins/ui/UiNode.hpp"
#include "plugins/ui/detail/UiLayout.hpp"
#include "plugins/ui/detail/UiNavigation.hpp"
#include "plugins/ui/detail/UiText.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"

using rtype::plugins::ui::UiAlign;
using rtype::plugins::ui::UiDirection;
using rtype::plugins::ui::UiDisplay;
using rtype::plugins::ui::UiJustify;
using rtype::plugins::ui::UiLength;
using rtype::plugins::ui::UiNode;
using rtype::plugins::ui::UiOverflow;
using rtype::plugins::ui::UiPosition;
using rtype::plugins::ui::detail::layoutNodes;
using rtype::plugins::ui::detail::UiLaidOutNode;
using rtype::plugins::ui::detail::UiLayoutNode;

static_assert(rtype::ecs::Component<UiNode>);

namespace {
constexpr glm::vec2 kView{800.0F, 600.0F};

/// @return The node @p index of @p placed.
const UiLaidOutNode& find(const std::vector<UiLaidOutNode>& placed, const std::size_t index) {
    for (const UiLaidOutNode& node : placed) {
        if (node.node == index) {
            return node;
        }
    }
    throw std::runtime_error{"the node is not in the layout"};
}
}  // namespace

TEST(UiLayout, ARootWithoutSizeFillsTheWidthOfTheView) {
    const auto placed = layoutNodes({UiLayoutNode{.style = UiNode{.height = UiLength::px(50.0F)}}}, {0}, kView);
    ASSERT_EQ(placed.size(), 1U);
    EXPECT_FLOAT_EQ(placed[0].size.x, 800.0F);
    EXPECT_FLOAT_EQ(placed[0].size.y, 50.0F);
}

TEST(UiLayout, PercentagesCountFromTheView) {
    const auto placed = layoutNodes(
        {UiLayoutNode{.style = UiNode{.width = UiLength::percent(50.0F), .height = UiLength::percent(25.0F)}}}, {0},
        kView);
    EXPECT_FLOAT_EQ(placed[0].size.x, 400.0F);
    EXPECT_FLOAT_EQ(placed[0].size.y, 150.0F);
}

TEST(UiLayout, ARowSharesItsWidthByGrow) {
    const std::vector<UiLayoutNode> nodes{
        UiLayoutNode{.style = UiNode{.direction = UiDirection::kRow, .width = UiLength::px(300.0F)},
                     .children = {1, 2}},
        UiLayoutNode{.style = UiNode{.grow = 1.0F}},
        UiLayoutNode{.style = UiNode{.grow = 2.0F}},
    };
    const auto placed = layoutNodes(nodes, {0}, kView);
    EXPECT_FLOAT_EQ(find(placed, 1).size.x, 100.0F);
    EXPECT_FLOAT_EQ(find(placed, 2).size.x, 200.0F);
    EXPECT_FLOAT_EQ(find(placed, 2).position.x, 100.0F);
}

TEST(UiLayout, CentersWithJustifyAndAlign) {
    const std::vector<UiLayoutNode> nodes{
        UiLayoutNode{.style = UiNode{.justify = UiJustify::kCenter,
                                     .alignItems = UiAlign::kCenter,
                                     .width = UiLength::percent(100.0F),
                                     .height = UiLength::percent(100.0F)},
                     .children = {1}},
        UiLayoutNode{.style = UiNode{.width = UiLength::px(100.0F), .height = UiLength::px(50.0F)}},
    };
    const auto placed = layoutNodes(nodes, {0}, kView);
    EXPECT_FLOAT_EQ(find(placed, 1).position.x, 350.0F);
    EXPECT_FLOAT_EQ(find(placed, 1).position.y, 275.0F);
}

TEST(UiLayout, AnAbsoluteNodeWithInsetsReplacesAnAnchor) {
    const auto placed = layoutNodes({UiLayoutNode{.style = UiNode{.position = UiPosition::kAbsolute,
                                                                  .width = UiLength::px(100.0F),
                                                                  .height = UiLength::px(40.0F),
                                                                  .right = UiLength::px(10.0F),
                                                                  .bottom = UiLength::px(20.0F)}}},
                                    {0}, kView);
    EXPECT_FLOAT_EQ(placed[0].position.x, 690.0F);
    EXPECT_FLOAT_EQ(placed[0].position.y, 540.0F);
}

TEST(UiLayout, KeepsThePaddingForWhatDrawsInside) {
    const auto placed = layoutNodes(
        {UiLayoutNode{.style = UiNode{.paddingLeft = UiLength::px(8.0F), .paddingTop = UiLength::px(4.0F)}}}, {0},
        kView);
    EXPECT_FLOAT_EQ(placed[0].padding.x, 8.0F);
    EXPECT_FLOAT_EQ(placed[0].padding.y, 4.0F);
}

TEST(UiLayout, MeasuresALeafByItsContent) {
    UiLayoutNode leaf;
    leaf.measure = [](float /*available*/) { return glm::vec2{120.0F, 30.0F}; };
    const std::vector<UiLayoutNode> nodes{
        UiLayoutNode{.style = UiNode{.alignItems = UiAlign::kFlexStart}, .children = {1}}, leaf};
    const auto placed = layoutNodes(nodes, {0}, kView);
    EXPECT_FLOAT_EQ(find(placed, 1).size.x, 120.0F);
    EXPECT_FLOAT_EQ(find(placed, 1).size.y, 30.0F);
}

TEST(UiLayout, DrawsParentsFirstAndSiblingsByZIndex) {
    const std::vector<UiLayoutNode> nodes{
        UiLayoutNode{.children = {1, 2}},
        UiLayoutNode{.style = UiNode{.zIndex = 5}},
        UiLayoutNode{.style = UiNode{.zIndex = 1}},
    };
    const auto placed = layoutNodes(nodes, {0}, kView);
    ASSERT_EQ(placed.size(), 3U);
    EXPECT_EQ(placed[0].node, 0U);
    EXPECT_EQ(placed[1].node, 2U);
    EXPECT_EQ(placed[2].node, 1U);
}

TEST(UiLayout, LeavesOutWhatIsNotDisplayed) {
    const std::vector<UiLayoutNode> nodes{
        UiLayoutNode{.children = {1}},
        UiLayoutNode{.style = UiNode{.display = UiDisplay::kNone}, .children = {2}},
        UiLayoutNode{},
    };
    EXPECT_EQ(layoutNodes(nodes, {0}, kView).size(), 1U);
}

TEST(UiLayout, ClipsTheChildrenOfAHiddenOverflow) {
    const std::vector<UiLayoutNode> nodes{
        UiLayoutNode{
            .style =
                UiNode{.overflow = UiOverflow::kHidden, .width = UiLength::px(100.0F), .height = UiLength::px(50.0F)},
            .children = {1}},
        UiLayoutNode{.style = UiNode{.height = UiLength::px(300.0F)}},
    };
    const auto placed = layoutNodes(nodes, {0}, kView);
    EXPECT_FLOAT_EQ(find(placed, 0).clip.w, 600.0F);
    EXPECT_FLOAT_EQ(find(placed, 1).clip.z, 100.0F);
    EXPECT_FLOAT_EQ(find(placed, 1).clip.w, 50.0F);
    EXPECT_FLOAT_EQ(find(placed, 0).contentSize.y, 300.0F);
}

TEST(UiLayout, ScrollMovesTheChildren) {
    const std::vector<UiLayoutNode> nodes{
        UiLayoutNode{.style = UiNode{.overflow = UiOverflow::kScroll, .height = UiLength::px(50.0F)},
                     .children = {1},
                     .scroll = {0.0F, 30.0F}},
        UiLayoutNode{.style = UiNode{.height = UiLength::px(300.0F)}},
    };
    EXPECT_FLOAT_EQ(find(layoutNodes(nodes, {0}, kView), 1).position.y, -30.0F);
}

TEST(UiNavigation, PrefersTheNextItemOfTheColumn) {
    const std::vector<glm::vec2> centers{{0.0F, 0.0F}, {0.0F, 100.0F}, {60.0F, 50.0F}};
    EXPECT_EQ(rtype::plugins::ui::detail::findNext(centers, {0.0F, 0.0F}, {0.0F, 1.0F}), 1U);
}

TEST(UiNavigation, FindsNothingPastTheEdge) {
    const std::vector<glm::vec2> centers{{0.0F, 0.0F}, {0.0F, 100.0F}};
    EXPECT_FALSE(rtype::plugins::ui::detail::findNext(centers, {0.0F, 100.0F}, {0.0F, 1.0F}));
}

TEST(UiText, EncodesUtf8) {
    EXPECT_EQ(rtype::plugins::ui::detail::encodeUtf8(U'a'), "a");
    EXPECT_EQ(rtype::plugins::ui::detail::encodeUtf8(U'\u00e9'), "\xC3\xA9");
    EXPECT_EQ(rtype::plugins::ui::detail::encodeUtf8(U'\U0001F600'), "\xF0\x9F\x98\x80");
}

TEST(UiText, MovesByWholeCharacters) {
    const std::string text = "a\xC3\xA9";
    EXPECT_EQ(rtype::plugins::ui::detail::previousCharacter(text, 3), 1U);
    EXPECT_EQ(rtype::plugins::ui::detail::nextCharacter(text, 1), 3U);
    EXPECT_EQ(rtype::plugins::ui::detail::previousCharacter(text, 0), 0U);
    EXPECT_EQ(rtype::plugins::ui::detail::nextCharacter(text, 3), 3U);
}
