/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Blend
*/

#pragma once

#include <cstdint>

#include "plugins/backend/graphics/BlendMode.hpp"
#include "render/vulkan/Export.hpp"

namespace rtype::render::vulkan::rendering {

/// @brief The blend modes as shaders read them: the CPU side of shaders/include/Blend.glsl, which must match it value
/// by value. Change both files together.
/// @{
inline constexpr std::uint32_t kBlendAlpha = 0;
inline constexpr std::uint32_t kBlendAdditive = 1;
inline constexpr std::uint32_t kBlendCutout = 2;
/// @}

static_assert(static_cast<std::uint32_t>(plugins::backend::graphics::BlendMode::kAlpha) == kBlendAlpha &&
                  static_cast<std::uint32_t>(plugins::backend::graphics::BlendMode::kAdditive) == kBlendAdditive &&
                  static_cast<std::uint32_t>(plugins::backend::graphics::BlendMode::kCutout) == kBlendCutout,
              "graphics::BlendMode must keep the values of shaders/include/Blend.glsl");

/// @return @p mode as shaders/include/Blend.glsl reads it.
[[nodiscard]] constexpr std::uint32_t toShaderBlend(plugins::backend::graphics::BlendMode mode) noexcept {
    return static_cast<std::uint32_t>(mode);
}

/// @brief A sprite instance's blend word also says how its texture is read: bits 0 to 7 hold the blend mode, bit 8
/// kShaderMsdf, and bits 16 to 31 the distance range of an MSDF font atlas, in 1/256 of an atlas pixel.
/// @{
inline constexpr std::uint32_t kBlendMask = 0xFFU;
inline constexpr std::uint32_t kShaderMsdf = 1U << 8U;
inline constexpr std::uint32_t kDistanceRangeShift = 16;
inline constexpr float kDistanceRangeUnit = 256.0F;
/// @}

/// @return @p mode, for a glyph of an MSDF font atlas whose distance range is @p distanceRange atlas pixels (0 to
/// 255, clamped).
[[nodiscard]] RTYPE_RENDER_VULKAN_API std::uint32_t toShaderMsdf(plugins::backend::graphics::BlendMode mode,
                                                                 float distanceRange) noexcept;

}  // namespace rtype::render::vulkan::rendering
