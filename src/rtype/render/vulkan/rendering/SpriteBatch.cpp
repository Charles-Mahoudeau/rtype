/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteBatch
*/

#include "SpriteBatch.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "Blend.hpp"
#include "plugins/backend/graphics/Sprite.hpp"
#include "plugins/backend/graphics/Texture.hpp"

namespace rtype::render::vulkan::rendering {

SpriteInstance SpriteInstance::fromSprite(const plugins::backend::graphics::Sprite& sprite,
                                          glm::uvec2 textureSize) noexcept {
    const glm::vec2 sourcePosition{sprite.source.getX(), sprite.source.getY()};
    const glm::vec2 sourceSize{sprite.source.getWidth(), sprite.source.getHeight()};
    const glm::vec2 texture{textureSize};
    const glm::vec2 uvMin = sourcePosition / texture;
    const glm::vec2 uvMax = (sourcePosition + sourceSize) / texture;
    return SpriteInstance{
        .position = sprite.position,
        .size = sourceSize * sprite.scale,
        .origin = sprite.origin * sprite.scale,
        .rotation = sprite.rotation,
        .blend = toShaderBlend(sprite.blend),
        .uvRect = {uvMin.x, uvMin.y, uvMax.x, uvMax.y},
        .color = sprite.color,
    };
}

std::array<vk::VertexInputBindingDescription, 1> SpriteInstance::getBindings(std::uint32_t binding) noexcept {
    return {vk::VertexInputBindingDescription{binding, sizeof(SpriteInstance), vk::VertexInputRate::eInstance}};
}

std::array<vk::VertexInputAttributeDescription, 7> SpriteInstance::getAttributes(std::uint32_t binding) noexcept {
    return {
        vk::VertexInputAttributeDescription{0, binding, vk::Format::eR32G32Sfloat, offsetof(SpriteInstance, position)},
        vk::VertexInputAttributeDescription{1, binding, vk::Format::eR32G32Sfloat, offsetof(SpriteInstance, size)},
        vk::VertexInputAttributeDescription{2, binding, vk::Format::eR32G32Sfloat, offsetof(SpriteInstance, origin)},
        vk::VertexInputAttributeDescription{3, binding, vk::Format::eR32Sfloat, offsetof(SpriteInstance, rotation)},
        vk::VertexInputAttributeDescription{4, binding, vk::Format::eR32G32B32A32Sfloat,
                                            offsetof(SpriteInstance, uvRect)},
        vk::VertexInputAttributeDescription{5, binding, vk::Format::eR32G32B32A32Sfloat,
                                            offsetof(SpriteInstance, color)},
        vk::VertexInputAttributeDescription{6, binding, vk::Format::eR32Uint, offsetof(SpriteInstance, blend)},
    };
}

void SpriteBatch::add(plugins::backend::graphics::TextureId texture, const SpriteInstance& instance) {
    if (_batches.empty() || _batches.back().texture != texture) {
        _batches.push_back(Batch{.texture = texture, .firstInstance = static_cast<std::uint32_t>(_instances.size())});
    }
    _instances.push_back(instance);
    ++_batches.back().instanceCount;
}

void SpriteBatch::clear() noexcept {
    _instances.clear();
    _batches.clear();
}

}  // namespace rtype::render::vulkan::rendering
