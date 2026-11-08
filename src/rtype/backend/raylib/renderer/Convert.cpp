/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Convert
*/

#include "Convert.hpp"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <glm/ext/vector_float2.hpp>

#include "rtype/contract/renderer/drawing/BlendMode.hpp"
#include "rtype/contract/renderer/drawing/Color.hpp"
#include "rtype/contract/renderer/math/Rect.hpp"

namespace rtype::backend::raylib::renderer {
namespace {

/// @return A linear channel in [0, 1] encoded with the sRGB transfer function.
float encodeSrgb(float linear) {
    constexpr float kLinearEnd = 0.0031308F;
    constexpr float kLinearSlope = 12.92F;
    constexpr float kScale = 1.055F;
    constexpr float kOffset = 0.055F;
    constexpr float kExponent = 1.0F / 2.4F;
    const float value = std::clamp(linear, 0.0F, 1.0F);
    return value <= kLinearEnd ? value * kLinearSlope : (kScale * std::pow(value, kExponent)) - kOffset;
}

/// @return A channel in [0, 1] as a byte.
unsigned char toByte(float value) {
    constexpr float kMax = 255.0F;
    return static_cast<unsigned char>(std::lround(std::clamp(value, 0.0F, 1.0F) * kMax));
}

}  // namespace

::Color toColor(const contract::renderer::Color& color) {
    return {.r = toByte(encodeSrgb(color.x)),
            .g = toByte(encodeSrgb(color.y)),
            .b = toByte(encodeSrgb(color.z)),
            .a = toByte(color.w)};
}

::Vector2 toVector(glm::vec2 value) { return {.x = value.x, .y = value.y}; }

::Rectangle toRectangle(const contract::renderer::math::Rect& rect) {
    return {.x = rect.getX(), .y = rect.getY(), .width = rect.getWidth(), .height = rect.getHeight()};
}

::Rectangle sourceOrWhole(const contract::renderer::math::Rect& rect, const ::Texture2D& texture) {
    if (rect.getWidth() == 0.0F || rect.getHeight() == 0.0F) {
        return {.x = 0.0F,
                .y = 0.0F,
                .width = static_cast<float>(texture.width),
                .height = static_cast<float>(texture.height)};
    }
    return toRectangle(rect);
}

int toBlend(contract::renderer::BlendMode blend) {
    return blend == contract::renderer::BlendMode::kAdditive ? BLEND_ADDITIVE : BLEND_ALPHA;
}

glm::vec2 rotate(glm::vec2 point, float angle) {
    const float cosine = std::cos(angle);
    const float sine = std::sin(angle);
    return {(point.x * cosine) - (point.y * sine), (point.x * sine) + (point.y * cosine)};
}

}  // namespace rtype::backend::raylib::renderer
