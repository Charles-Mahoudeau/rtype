/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Convert
*/

#pragma once

#include <raylib.h>

#include <glm/ext/vector_float2.hpp>

#include "rtype/contract/renderer/drawing/BlendMode.hpp"
#include "rtype/contract/renderer/drawing/Color.hpp"
#include "rtype/contract/renderer/math/Rect.hpp"

/// The conversions from the renderer contract's types to raylib's.
namespace rtype::backend::raylib::renderer {

/// @brief Turns radians (the contract's angles) into degrees (raylib's).
inline constexpr float kDegreesPerRadian = 57.29577951308232F;

/// @return A colour of the engine, linear like the Vulkan renderer's, encoded in sRGB as raylib writes it as is: both
/// renderers then show the same colours.
[[nodiscard]] ::Color toColor(const contract::renderer::Color& color);

/// @return @p value as a raylib vector.
[[nodiscard]] ::Vector2 toVector(glm::vec2 value);

/// @return @p rect as a raylib rectangle.
[[nodiscard]] ::Rectangle toRectangle(const contract::renderer::math::Rect& rect);

/// @return @p rect, or the whole @p texture when @p rect is empty: what a source rectangle of zero size means.
[[nodiscard]] ::Rectangle sourceOrWhole(const contract::renderer::math::Rect& rect, const ::Texture2D& texture);

/// @return The raylib blend mode of @p blend: additive, or alpha (kCutout is drawn as alpha).
[[nodiscard]] int toBlend(contract::renderer::BlendMode blend);

/// @return @p point turned by @p angle (radians, clockwise on screen with Y down).
[[nodiscard]] glm::vec2 rotate(glm::vec2 point, float angle);

}  // namespace rtype::backend::raylib::renderer
