/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Blend
*/

#pragma once

#include <cstdint>

#include "plugins/backend/graphics/BlendMode.hpp"

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

}  // namespace rtype::render::vulkan::rendering
