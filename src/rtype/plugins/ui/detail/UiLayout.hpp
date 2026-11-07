/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UiLayout
*/

#pragma once

#include <cstddef>
#include <functional>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <vector>

#include "plugins/ui/Export.hpp"
#include "plugins/ui/UiNode.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::plugins::ui::detail {

/// @brief Gives the size of a node's content (a text) for a width: `available`, or free if negative.
using UiMeasure = std::function<glm::vec2(float available)>;

/// @brief One node to lay out. Its children are indices in the same list.
struct UiLayoutNode {
    ecs::Entity entity;                 ///< What it comes from.
    UiNode style;                       ///< Its flexbox properties.
    std::vector<std::size_t> children;  ///< In the order of the hierarchy.
    UiMeasure measure;                  ///< Sizes a leaf by its content; empty: it has none.
    glm::vec2 scroll{0.0F};             ///< Moves its children up and left.
};

/// @brief Where a node ended up.
struct UiLaidOutNode {
    ecs::Entity entity;           ///< What it comes from.
    std::size_t node = 0;         ///< Its index in the list given.
    glm::vec2 position{0.0F};     ///< Its top-left corner, in view pixels.
    glm::vec2 size{0.0F};         ///< Its size.
    glm::vec2 contentSize{0.0F};  ///< What its children take from its top-left corner, before the scroll.
    glm::vec4 clip{0.0F};         ///< What it is visible in: x, y, width, height.
    glm::vec4 padding{0.0F};      ///< Its padding, in pixels: left, top, right, bottom.
};

/// @brief Lays out @p nodes with Yoga, each of @p roots over the whole of @p view.
/// @return The nodes shown, in draw order: parents first, siblings by zIndex then in order. A node with
/// UiDisplay::kNone is left out, with its children.
[[nodiscard]] RTYPE_PLUGIN_UI_API std::vector<UiLaidOutNode> layoutNodes(const std::vector<UiLayoutNode>& nodes,
                                                                         const std::vector<std::size_t>& roots,
                                                                         glm::vec2 view);

}  // namespace rtype::plugins::ui::detail
