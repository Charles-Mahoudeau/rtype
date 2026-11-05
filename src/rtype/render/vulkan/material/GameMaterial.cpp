/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GameMaterial
*/

#include "GameMaterial.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <vulkan/vulkan_raii.hpp>

#include "plugins/backend/graphics/Shader.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/descriptor/DescriptorAllocator.hpp"
#include "render/vulkan/descriptor/DescriptorSetLayout.hpp"
#include "render/vulkan/descriptor/DescriptorWriter.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"

namespace rtype::render::vulkan::material {

GameMaterial::GameMaterial(const memory::Allocator& allocator, descriptor::DescriptorAllocator& descriptors,
                           const descriptor::DescriptorSetLayout& layout, std::uint32_t materialSize,
                           const plugins::backend::graphics::MaterialDesc& desc, std::size_t framesInFlight)
    : _shader{desc.shader}, _values{desc.values}, _textures{desc.textures} {
    _frames.reserve(framesInFlight);
    for (std::size_t frame = 0; frame < framesInFlight; ++frame) {
        Frame& created = _frames.emplace_back(Frame{.set = descriptors.allocate(layout)});
        allocator.getDevice().setName(*created.set, std::format("Material set, frame {}", frame));
        if (materialSize > 0) {
            created.values.emplace(allocator, materialSize, vk::BufferUsageFlagBits::eUniformBuffer,
                                   memory::Allocator::MemoryAccess::kStaging);
            allocator.getDevice().setName(*created.values->getBuffer(),
                                          std::format("Material values, frame {}", frame));
        }
    }
}

void GameMaterial::update(const plugins::backend::graphics::MaterialDesc& desc) {
    _values = desc.values;
    _textures = desc.textures;
    ++_version;
}

vk::DescriptorSet GameMaterial::prepare(const core::Device& device, std::size_t frameIndex,
                                        const TextureFinder& findTexture,
                                        plugins::backend::graphics::TextureId fallback) {
    Frame& frame = _frames.at(frameIndex);
    descriptor::DescriptorWriter writer;
    bool written = false;
    if (frame.version != _version) {
        if (frame.values) {
            frame.values->write(_values);
            if (frame.version == 0) {
                writer.writeBuffer(kMaterialValuesBinding, *frame.values->getBuffer(), 0, frame.values->getSize());
                written = true;
            }
        }
        frame.version = _version;
    }
    for (std::size_t slot = 0; slot < _textures.size(); ++slot) {
        plugins::backend::graphics::TextureId texture = _textures.at(slot);
        std::optional<TextureBinding> binding =
            texture == plugins::backend::graphics::TextureId{} ? std::nullopt : findTexture(texture);
        if (!binding) {
            texture = fallback;
            binding = findTexture(fallback);
        }
        if (binding && frame.bound.at(slot) != texture) {
            writer.writeImage(kMaterialFirstTextureBinding + static_cast<std::uint32_t>(slot), binding->view,
                              binding->sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
            frame.bound.at(slot) = texture;
            written = true;
        }
    }
    if (written) {
        writer.update(device, *frame.set);
    }
    return *frame.set;
}

}  // namespace rtype::render::vulkan::material
