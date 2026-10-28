/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Blend
*/

#include "Blend.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "plugins/backend/graphics/BlendMode.hpp"

namespace rtype::render::vulkan::rendering {

namespace {
/// @brief The largest distance range the 16 bits hold, in 1/256 of a pixel.
constexpr float kMaxDistanceRange = 65535.0F;
}  // namespace

std::uint32_t toShaderMsdf(plugins::backend::graphics::BlendMode mode, float distanceRange) noexcept {
    const float units = std::clamp(distanceRange * kDistanceRangeUnit, 0.0F, kMaxDistanceRange);
    return toShaderBlend(mode) | kShaderMsdf | (static_cast<std::uint32_t>(std::lround(units)) << kDistanceRangeShift);
}

}  // namespace rtype::render::vulkan::rendering
