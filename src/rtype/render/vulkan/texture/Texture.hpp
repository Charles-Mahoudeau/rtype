/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Texture
*/

#pragma once

#include "plugins/backend/graphics/Texture.hpp"
#include "render/vulkan/memory/Image.hpp"

namespace rtype::render::vulkan::texture {

/// @brief A texture of the renderer, behind a plugins::backend::graphics::TextureId: its image, sampled by the
/// shaders, and how it was asked to be sampled.
struct Texture {
    memory::Image image;                           ///< RGBA8 sRGB, in SHADER_READ_ONLY_OPTIMAL once uploaded.
    plugins::backend::graphics::TextureDesc desc;  ///< Its size, filter and repeat mode (the sampler to use).
};

}  // namespace rtype::render::vulkan::texture
