/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderLayout
*/

#pragma once

#include <cstdint>

/// @brief Where shaders find their data, grouped by how often it changes (the C++ side of
/// shaders/include/ShaderLayout.glsl, which must match it):
///
/// | What | Where | Changes | GLSL |
/// |---|---|---|---|
/// | FrameUniforms (time, framebuffer size, later the camera) | set 0, binding 0 | once per frame | `frame` |
/// | The texture and its sampler | set 1, binding 0 | per texture (batch) | `uTexture` |
/// | DrawConstants (transform, color, UV rect) | push constants | per draw | `draw` |
///
/// Binding the sets from the least to the most frequently changed keeps the lower ones bound: a new texture rebinds
/// set 1 only. Per-object data fits in the push constants, so there is no set 2.
namespace rtype::render::vulkan::rendering {

/// @brief Set of the per-frame data: FrameUniforms.
inline constexpr std::uint32_t kFrameSet = 0;
/// @brief Binding of the FrameUniforms uniform buffer in kFrameSet.
inline constexpr std::uint32_t kFrameUniformsBinding = 0;
/// @brief Set of the texture being drawn.
inline constexpr std::uint32_t kTextureSet = 1;
/// @brief Binding of the texture's combined image sampler in kTextureSet.
inline constexpr std::uint32_t kTextureBinding = 0;
/// @brief Push constant bytes every device supports: DrawConstants must fit in them.
inline constexpr std::uint32_t kMaxPushConstantsSize = 128;

}  // namespace rtype::render::vulkan::rendering
