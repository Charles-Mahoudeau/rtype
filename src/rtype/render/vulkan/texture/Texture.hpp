/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Texture
*/

#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "plugins/backend/graphics/Texture.hpp"
#include "render/vulkan/memory/Image.hpp"

namespace rtype::render::vulkan::texture {

/// @brief A texture of the renderer, behind a plugins::backend::graphics::TextureId: its image, how it is sampled,
/// and the descriptor set that hands both to the shaders.
struct Texture {
    memory::Image image;                           ///< RGBA8 sRGB, in SHADER_READ_ONLY_OPTIMAL once uploaded.
    plugins::backend::graphics::TextureDesc desc;  ///< Its size, filter and repeat mode (the sampler to use).
    vk::raii::DescriptorSet set;                   ///< Set 1: the image and its sampler, at binding 0.
};

}  // namespace rtype::render::vulkan::texture
