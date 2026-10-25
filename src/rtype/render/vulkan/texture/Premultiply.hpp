/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Premultiply
*/

#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "render/vulkan/Export.hpp"

namespace rtype::render::vulkan::texture {

/// @brief Multiplies the colour of RGBA8 sRGB pixels by their alpha, as the renderer's premultiplied-alpha blending
/// expects its textures.
///
/// @details The product is taken in linear space, where the GPU filters and blends (the sampler decodes sRGB), then
/// encoded back to sRGB: multiplying the encoded bytes directly would darken every semi-transparent edge. Opaque pixels
/// are unchanged, fully transparent ones become (0, 0, 0, 0): that is what lets linear filtering blend a sprite's edge
/// with its transparent surroundings without a dark or light fringe.
/// @param rgba Pixels as IRenderer::createTexture() takes them: 4 bytes each, straight (not premultiplied) alpha.
/// @return The premultiplied pixels, as many bytes as @p rgba.
/// @throws std::invalid_argument If @p rgba's size is not a multiple of 4.
[[nodiscard]] RTYPE_RENDER_VULKAN_API std::vector<std::byte> premultiplySrgb(std::span<const std::byte> rgba);

}  // namespace rtype::render::vulkan::texture
