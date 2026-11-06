/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UiLayout
*/

#include "plugins/ui/detail/UiLayout.hpp"

#include <yoga/Yoga.h>

#include <algorithm>
#include <cstddef>
#include <glm/common.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <memory>
#include <vector>

#include "plugins/ui/UiNode.hpp"

namespace rtype::plugins::ui::detail {
namespace {
/// @brief Frees a Yoga configuration.
struct ConfigDeleter {
    void operator()(YGConfig* config) const noexcept { YGConfigFree(config); }
};

/// @brief Frees a Yoga tree.
struct NodeDeleter {
    void operator()(YGNode* node) const noexcept { YGNodeFreeRecursive(node); }
};

[[nodiscard]] YGFlexDirection toYoga(const UiDirection direction) noexcept {
    switch (direction) {
        case UiDirection::kColumnReverse:
            return YGFlexDirectionColumnReverse;
        case UiDirection::kRow:
            return YGFlexDirectionRow;
        case UiDirection::kRowReverse:
            return YGFlexDirectionRowReverse;
        case UiDirection::kColumn:
            break;
    }
    return YGFlexDirectionColumn;
}

[[nodiscard]] YGWrap toYoga(const UiWrap wrap) noexcept {
    switch (wrap) {
        case UiWrap::kWrap:
            return YGWrapWrap;
        case UiWrap::kWrapReverse:
            return YGWrapWrapReverse;
        case UiWrap::kNoWrap:
            break;
    }
    return YGWrapNoWrap;
}

[[nodiscard]] YGJustify toYoga(const UiJustify justify) noexcept {
    switch (justify) {
        case UiJustify::kCenter:
            return YGJustifyCenter;
        case UiJustify::kFlexEnd:
            return YGJustifyFlexEnd;
        case UiJustify::kSpaceBetween:
            return YGJustifySpaceBetween;
        case UiJustify::kSpaceAround:
            return YGJustifySpaceAround;
        case UiJustify::kSpaceEvenly:
            return YGJustifySpaceEvenly;
        case UiJustify::kFlexStart:
            break;
    }
    return YGJustifyFlexStart;
}

[[nodiscard]] YGAlign toYoga(const UiAlign align) noexcept {
    switch (align) {
        case UiAlign::kFlexStart:
            return YGAlignFlexStart;
        case UiAlign::kCenter:
            return YGAlignCenter;
        case UiAlign::kFlexEnd:
            return YGAlignFlexEnd;
        case UiAlign::kStretch:
            return YGAlignStretch;
        case UiAlign::kBaseline:
            return YGAlignBaseline;
        case UiAlign::kSpaceBetween:
            return YGAlignSpaceBetween;
        case UiAlign::kSpaceAround:
            return YGAlignSpaceAround;
        case UiAlign::kSpaceEvenly:
            return YGAlignSpaceEvenly;
        case UiAlign::kAuto:
            break;
    }
    return YGAlignAuto;
}

/// @brief Calls @p pixels, @p percent or @p automatic for @p length, the Yoga setters of one property.
template <typename Pixels, typename Percent, typename Automatic>
void applyLength(const UiLength length, const Pixels& pixels, const Percent& percent, const Automatic& automatic) {
    switch (length.getUnit()) {
        case UiUnit::kPixels:
            pixels(length.value);
            break;
        case UiUnit::kPercent:
            percent(length.value);
            break;
        case UiUnit::kAuto:
            automatic();
            break;
    }
}

void applyInset(YGNodeRef node, const YGEdge edge, const UiLength length) {
    applyLength(
        length, [&](float value) { YGNodeStyleSetPosition(node, edge, value); },
        [&](float value) { YGNodeStyleSetPositionPercent(node, edge, value); }, [] {});
}

void applyMargin(YGNodeRef node, const YGEdge edge, const UiLength length) {
    applyLength(
        length, [&](float value) { YGNodeStyleSetMargin(node, edge, value); },
        [&](float value) { YGNodeStyleSetMarginPercent(node, edge, value); }, [] {});
}

void applyPadding(YGNodeRef node, const YGEdge edge, const UiLength length) {
    applyLength(
        length, [&](float value) { YGNodeStyleSetPadding(node, edge, value); },
        [&](float value) { YGNodeStyleSetPaddingPercent(node, edge, value); }, [] {});
}

/// @brief Gives @p node the properties of @p style.
void applyStyle(YGNodeRef node, const UiNode& style) {
    YGNodeStyleSetPositionType(
        node, style.position == UiPosition::kAbsolute ? YGPositionTypeAbsolute : YGPositionTypeRelative);
    YGNodeStyleSetFlexDirection(node, toYoga(style.direction));
    YGNodeStyleSetFlexWrap(node, toYoga(style.wrap));
    YGNodeStyleSetJustifyContent(node, toYoga(style.justify));
    YGNodeStyleSetAlignItems(node, toYoga(style.alignItems));
    YGNodeStyleSetAlignSelf(node, toYoga(style.alignSelf));
    YGNodeStyleSetAlignContent(node, toYoga(style.alignContent));
    YGNodeStyleSetOverflow(node, style.overflow == UiOverflow::kVisible  ? YGOverflowVisible
                                 : style.overflow == UiOverflow::kHidden ? YGOverflowHidden
                                                                         : YGOverflowScroll);
    YGNodeStyleSetFlexGrow(node, style.grow);
    YGNodeStyleSetFlexShrink(node, style.shrink);
    applyLength(
        style.basis, [&](float value) { YGNodeStyleSetFlexBasis(node, value); },
        [&](float value) { YGNodeStyleSetFlexBasisPercent(node, value); }, [&] { YGNodeStyleSetFlexBasisAuto(node); });
    applyLength(
        style.width, [&](float value) { YGNodeStyleSetWidth(node, value); },
        [&](float value) { YGNodeStyleSetWidthPercent(node, value); }, [&] { YGNodeStyleSetWidthAuto(node); });
    applyLength(
        style.height, [&](float value) { YGNodeStyleSetHeight(node, value); },
        [&](float value) { YGNodeStyleSetHeightPercent(node, value); }, [&] { YGNodeStyleSetHeightAuto(node); });
    applyLength(
        style.minWidth, [&](float value) { YGNodeStyleSetMinWidth(node, value); },
        [&](float value) { YGNodeStyleSetMinWidthPercent(node, value); }, [] {});
    applyLength(
        style.minHeight, [&](float value) { YGNodeStyleSetMinHeight(node, value); },
        [&](float value) { YGNodeStyleSetMinHeightPercent(node, value); }, [] {});
    applyLength(
        style.maxWidth, [&](float value) { YGNodeStyleSetMaxWidth(node, value); },
        [&](float value) { YGNodeStyleSetMaxWidthPercent(node, value); }, [] {});
    applyLength(
        style.maxHeight, [&](float value) { YGNodeStyleSetMaxHeight(node, value); },
        [&](float value) { YGNodeStyleSetMaxHeightPercent(node, value); }, [] {});
    if (style.aspectRatio > 0.0F) {
        YGNodeStyleSetAspectRatio(node, style.aspectRatio);
    }
    applyInset(node, YGEdgeLeft, style.left);
    applyInset(node, YGEdgeTop, style.top);
    applyInset(node, YGEdgeRight, style.right);
    applyInset(node, YGEdgeBottom, style.bottom);
    applyMargin(node, YGEdgeLeft, style.marginLeft);
    applyMargin(node, YGEdgeTop, style.marginTop);
    applyMargin(node, YGEdgeRight, style.marginRight);
    applyMargin(node, YGEdgeBottom, style.marginBottom);
    applyPadding(node, YGEdgeLeft, style.paddingLeft);
    applyPadding(node, YGEdgeTop, style.paddingTop);
    applyPadding(node, YGEdgeRight, style.paddingRight);
    applyPadding(node, YGEdgeBottom, style.paddingBottom);
    YGNodeStyleSetBorder(node, YGEdgeAll, style.border);
    YGNodeStyleSetGap(node, YGGutterRow, style.rowGap);
    YGNodeStyleSetGap(node, YGGutterColumn, style.columnGap);
}

/// @brief Asks a node's UiMeasure for its size, as Yoga wants it.
YGSize measureNode(YGNodeConstRef node, const float width, const YGMeasureMode widthMode, const float /*height*/,
                   const YGMeasureMode /*heightMode*/) {
    const auto* measure = static_cast<const UiMeasure*>(YGNodeGetContext(node));
    const glm::vec2 size = (*measure)(widthMode == YGMeasureModeUndefined ? -1.0F : width);
    return YGSize{.width = size.x, .height = size.y};
}

/// @brief Builds the Yoga tree of the node @p index, in the order of the hierarchy.
[[nodiscard]] YGNodeRef build(const std::vector<UiLayoutNode>& nodes, const std::size_t index, YGConfigConstRef config,
                              std::vector<YGNodeRef>& built) {
    const UiLayoutNode& source = nodes.at(index);
    YGNodeRef node = YGNodeNewWithConfig(config);
    built.at(index) = node;
    applyStyle(node, source.style);
    std::size_t position = 0;
    for (const std::size_t child : source.children) {
        if (nodes.at(child).style.display != UiDisplay::kNone) {
            YGNodeInsertChild(node, build(nodes, child, config, built), position);
            ++position;
        }
    }
    // Yoga only measures leaves.
    if (source.measure && position == 0) {
        YGNodeSetContext(node,
                         const_cast<UiMeasure*>(&source.measure));  // NOLINT(cppcoreguidelines-pro-type-const-cast)
        YGNodeSetMeasureFunc(node, &measureNode);
    }
    return node;
}

/// @return The intersection of @p first and @p second, two rectangles x, y, width, height.
[[nodiscard]] glm::vec4 intersect(const glm::vec4& first, const glm::vec4& second) noexcept {
    const glm::vec2 start = glm::max(glm::vec2{first.x, first.y}, glm::vec2{second.x, second.y});
    const glm::vec2 end =
        glm::min(glm::vec2{first.x + first.z, first.y + first.w}, glm::vec2{second.x + second.z, second.y + second.w});
    const glm::vec2 size = glm::max(end - start, glm::vec2{0.0F});
    return glm::vec4{start.x, start.y, size.x, size.y};
}

/// @brief Adds the node @p index and what is under it to @p result, in draw order.
void collect(const std::vector<UiLayoutNode>& nodes, const std::vector<YGNodeRef>& built, const std::size_t index,
             const glm::vec2 origin, const glm::vec4& clip, std::vector<UiLaidOutNode>& result) {
    const UiLayoutNode& source = nodes.at(index);
    YGNodeRef node = built.at(index);
    const glm::vec2 position = origin + glm::vec2{YGNodeLayoutGetLeft(node), YGNodeLayoutGetTop(node)};
    const glm::vec2 size{YGNodeLayoutGetWidth(node), YGNodeLayoutGetHeight(node)};
    const std::size_t self = result.size();
    result.push_back(UiLaidOutNode{
        .entity = source.entity,
        .node = index,
        .position = position,
        .size = size,
        .clip = clip,
        .padding = glm::vec4{YGNodeLayoutGetPadding(node, YGEdgeLeft), YGNodeLayoutGetPadding(node, YGEdgeTop),
                             YGNodeLayoutGetPadding(node, YGEdgeRight), YGNodeLayoutGetPadding(node, YGEdgeBottom)}});

    glm::vec2 content{0.0F};
    std::vector<std::size_t> children;
    for (const std::size_t child : source.children) {
        if (nodes.at(child).style.display == UiDisplay::kNone) {
            continue;
        }
        children.push_back(child);
        YGNodeRef laidOut = built.at(child);
        content = glm::max(content, glm::vec2{YGNodeLayoutGetLeft(laidOut) + YGNodeLayoutGetWidth(laidOut),
                                              YGNodeLayoutGetTop(laidOut) + YGNodeLayoutGetHeight(laidOut)});
    }
    result.at(self).contentSize = content;
    std::ranges::stable_sort(children, {}, [&nodes](std::size_t child) { return nodes.at(child).style.zIndex; });
    const glm::vec4 inner = source.style.overflow == UiOverflow::kVisible
                                ? clip
                                : intersect(clip, glm::vec4{position.x, position.y, size.x, size.y});
    for (const std::size_t child : children) {
        collect(nodes, built, child, position - source.scroll, inner, result);
    }
}
}  // namespace

std::vector<UiLaidOutNode> layoutNodes(const std::vector<UiLayoutNode>& nodes, const std::vector<std::size_t>& roots,
                                       const glm::vec2 view) {
    const std::unique_ptr<YGConfig, ConfigDeleter> config{YGConfigNew()};
    YGConfigSetPointScaleFactor(config.get(), 0.0F);  // No rounding: the view may not be in physical pixels.
    std::vector<UiLaidOutNode> result;
    result.reserve(nodes.size());
    std::vector<std::size_t> sorted = roots;
    std::ranges::stable_sort(sorted, {}, [&nodes](std::size_t root) { return nodes.at(root).style.zIndex; });
    for (const std::size_t root : sorted) {
        if (nodes.at(root).style.display == UiDisplay::kNone) {
            continue;
        }
        std::vector<YGNodeRef> built(nodes.size(), nullptr);
        const std::unique_ptr<YGNode, NodeDeleter> tree{build(nodes, root, config.get(), built)};
        // Every root is laid out in a parent the size of the view, so its own lengths, percentages included, count.
        const std::unique_ptr<YGNode, NodeDeleter> frame{YGNodeNewWithConfig(config.get())};
        YGNodeStyleSetWidth(frame.get(), view.x);
        YGNodeStyleSetHeight(frame.get(), view.y);
        YGNodeInsertChild(frame.get(), tree.get(), 0);
        YGNodeCalculateLayout(frame.get(), view.x, view.y, YGDirectionLTR);
        collect(nodes, built, root, glm::vec2{0.0F}, glm::vec4{0.0F, 0.0F, view.x, view.y}, result);
        YGNodeRemoveChild(frame.get(), tree.get());
    }
    return result;
}
}  // namespace rtype::plugins::ui::detail
