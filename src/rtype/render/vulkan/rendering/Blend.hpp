/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Blend
*/

#pragma once

#include <cstdint>

#include "render/vulkan/Export.hpp"
#include "rtype/contract/renderer/drawing/BlendMode.hpp"

namespace rtype::render::vulkan::rendering {

/// @brief The blend modes as shaders read them: the CPU side of the shaders ABI's fragment_common.glsl
/// (contract/renderer/shaders/), which must match it value by value. Change both files together.
/// @{
inline constexpr std::uint32_t kBlendAlpha = 0;
inline constexpr std::uint32_t kBlendAdditive = 1;
inline constexpr std::uint32_t kBlendCutout = 2;
/// @}

static_assert(static_cast<std::uint32_t>(contract::renderer::BlendMode::kAlpha) == kBlendAlpha &&
                  static_cast<std::uint32_t>(contract::renderer::BlendMode::kAdditive) == kBlendAdditive &&
                  static_cast<std::uint32_t>(contract::renderer::BlendMode::kCutout) == kBlendCutout,
              "contract::renderer::BlendMode must keep the values of the shaders ABI's fragment_common.glsl");

/// @return @p mode as the shaders ABI's fragment_common.glsl reads it.
[[nodiscard]] constexpr std::uint32_t toShaderBlend(contract::renderer::BlendMode mode) noexcept {
    return static_cast<std::uint32_t>(mode);
}

/// @brief A glyph instance's blend word also holds the distance range of its MSDF font atlas: bits 0 to 7 hold the
/// blend mode, and bits 16 to 31 the distance range, in 1/256 of an atlas pixel, which the glyph shader reads.
/// @{
inline constexpr std::uint32_t kBlendMask = 0xFFU;
inline constexpr std::uint32_t kDistanceRangeShift = 16;
inline constexpr float kDistanceRangeUnit = 256.0F;
/// @}

/// @return @p mode, for a glyph of an MSDF font atlas whose distance range is @p distanceRange atlas pixels (0 to
/// 255, clamped).
[[nodiscard]] RTYPE_RENDER_VULKAN_API std::uint32_t toShaderMsdf(contract::renderer::BlendMode mode,
                                                                 float distanceRange) noexcept;

}  // namespace rtype::render::vulkan::rendering
