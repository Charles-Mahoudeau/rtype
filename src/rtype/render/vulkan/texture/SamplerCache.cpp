/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SamplerCache
*/

#include "SamplerCache.hpp"

#include <algorithm>
#include <cstddef>
#include <vulkan/vulkan_raii.hpp>

#include "plugins/backend/graphics/Texture.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/core/PhysicalDevice.hpp"

namespace rtype::render::vulkan::texture {

namespace {

/// @brief Most anisotropy a linear sampler asks for: beyond 16x, no visible gain.
constexpr float kMaxAnisotropy = 16.0F;

/// @return A sampler reading with @p filter, repeating or clamping; anisotropic if @p anisotropy is above 1.
vk::raii::Sampler createSampler(const core::Device& device, plugins::backend::graphics::TextureFilter filter,
                                bool repeat, float anisotropy) {
    const bool linear = filter == plugins::backend::graphics::TextureFilter::kLinear;
    const vk::Filter vkFilter = linear ? vk::Filter::eLinear : vk::Filter::eNearest;
    const vk::SamplerAddressMode address =
        repeat ? vk::SamplerAddressMode::eRepeat : vk::SamplerAddressMode::eClampToEdge;
    const bool anisotropic = linear && anisotropy > 1.0F;
    return vk::raii::Sampler{device.getDevice(), vk::SamplerCreateInfo{}
                                                     .setMagFilter(vkFilter)
                                                     .setMinFilter(vkFilter)
                                                     .setMipmapMode(linear ? vk::SamplerMipmapMode::eLinear
                                                                           : vk::SamplerMipmapMode::eNearest)
                                                     .setAddressModeU(address)
                                                     .setAddressModeV(address)
                                                     .setAddressModeW(address)
                                                     .setAnisotropyEnable(anisotropic ? vk::True : vk::False)
                                                     .setMaxAnisotropy(anisotropic ? anisotropy : 1.0F)
                                                     .setMinLod(0.0F)
                                                     .setMaxLod(0.0F)};
}

/// @return The anisotropy linear samplers use: up to kMaxAnisotropy when the device enabled it, 1 (none) otherwise.
float anisotropyOf(const core::PhysicalDevice& physicalDevice) {
    return physicalDevice.getOptionalFeatures().samplerAnisotropy
               ? std::min(kMaxAnisotropy, physicalDevice.getLimits().maxSamplerAnisotropy)
               : 1.0F;
}

}  // namespace

SamplerCache::SamplerCache(const core::Device& device, const core::PhysicalDevice& physicalDevice)
    : _samplers{createSampler(device, plugins::backend::graphics::TextureFilter::kNearest, false,
                              anisotropyOf(physicalDevice)),
                createSampler(device, plugins::backend::graphics::TextureFilter::kNearest, true,
                              anisotropyOf(physicalDevice)),
                createSampler(device, plugins::backend::graphics::TextureFilter::kLinear, false,
                              anisotropyOf(physicalDevice)),
                createSampler(device, plugins::backend::graphics::TextureFilter::kLinear, true,
                              anisotropyOf(physicalDevice))} {}

const vk::raii::Sampler& SamplerCache::get(plugins::backend::graphics::TextureFilter filter, bool repeat) const {
    return _samplers.at(indexOf(filter, repeat));
}

std::size_t SamplerCache::indexOf(plugins::backend::graphics::TextureFilter filter, bool repeat) {
    const std::size_t linear = filter == plugins::backend::graphics::TextureFilter::kLinear ? 1 : 0;
    return (linear * 2) + (repeat ? 1 : 0);
}

}  // namespace rtype::render::vulkan::texture
