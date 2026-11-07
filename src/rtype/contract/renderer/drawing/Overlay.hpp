/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Overlay
*/

#pragma once

#include <optional>
#include <variant>

#include "rtype/contract/renderer/drawing/Shape.hpp"
#include "rtype/contract/renderer/drawing/Sprite.hpp"
#include "rtype/contract/renderer/drawing/Text.hpp"
#include "rtype/contract/renderer/math/Rect.hpp"

namespace rtype::contract::renderer {

/// @brief One drawing of the overlay, given to IRenderer::drawOverlay().
struct OverlayItem {
    std::variant<Sprite, Shape, Text> drawing;  ///< What to draw, placed in view pixels.
    std::optional<math::Rect> clip;             ///< The part of the view it stays in, in view pixels; nothing: all.
};

}  // namespace rtype::contract::renderer
