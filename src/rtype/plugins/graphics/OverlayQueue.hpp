/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** OverlayQueue
*/

#pragma once

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <utility>
#include <vector>

#include "rtype/contract/renderer/drawing/Overlay.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::graphics {

/// @brief What the frame draws over everything else, in view pixels, as the ECS resource "rtype.OverlayQueue": the
/// GraphicsPlugin hands it to IRenderer::drawOverlay() just before it ends the frame.
///
/// @details Render systems push to it after "rtype.graphics.camera" and before "rtype.graphics.endFrame". They take it
/// as a ResAppend and never touch the renderer, so they need no order with the other plugins that draw. Items stay in
/// the order pushed.
class OverlayQueue {
  public:
    OverlayQueue() = default;
    ~OverlayQueue() = default;
    OverlayQueue(const OverlayQueue& other) = default;
    OverlayQueue& operator=(const OverlayQueue& other) = default;
    OverlayQueue(OverlayQueue&& other) noexcept = default;
    OverlayQueue& operator=(OverlayQueue&& other) noexcept = default;

    /// @brief Adds @p item on top of what was pushed.
    void push(contract::renderer::OverlayItem item) { _items.push_back(std::move(item)); }

    /// @return The items pushed since the last call, in order, leaving the queue empty.
    [[nodiscard]] std::vector<contract::renderer::OverlayItem> take() { return std::exchange(_items, {}); }

  private:
    std::vector<contract::renderer::OverlayItem> _items;  ///< In draw order.
};
RTYPE_ECS_RESOURCE(OverlayQueue, "rtype.OverlayQueue");

/// @brief The sizes of the frame, as the ECS resource "rtype.ViewInfo", written each frame before the Render systems
/// that draw: what a plugin needs to place things on screen without touching the backend.
struct ViewInfo {
    glm::vec2 viewSize{0.0F};   ///< The view: what overlay items are placed in, in view pixels.
    glm::uvec2 framebuffer{0};  ///< The framebuffer, in pixels; 0x0 while minimized.
    glm::uvec2 window{0};       ///< The window, in the units of the mouse events.
};
RTYPE_ECS_RESOURCE(ViewInfo, "rtype.ViewInfo");

}  // namespace rtype::plugins::graphics
