/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SamplerCache
*/

#pragma once

#include <array>
#include <cstddef>
#include <vulkan/vulkan_raii.hpp>

#include "plugins/backend/graphics/Texture.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/core/PhysicalDevice.hpp"

namespace rtype::render::vulkan::texture {

/// @brief The samplers of the renderer, one per way a texture can be read: every texture with the same filter and
/// repeat mode shares one.
///
/// @details A sampler only says how a shader reads an image (filtering, what happens past its edges), so a few cover
/// every texture. All of them are created at once:
/// - nearest: sharp pixels when scaled up (pixel art); linear: smooth interpolation, anisotropic when the device
///   supports it (up to 16x, sharper at grazing angles);
/// - clamp: past an edge, the edge's texel; repeat: the texture tiles.
/// Every sampler reads the single mip level the textures have.
/// @warning Destroy it before the Device, and after every descriptor set that points to its samplers.
class RTYPE_RENDER_VULKAN_API SamplerCache {
  public:
    /// @param device Creates the samplers.
    /// @param physicalDevice Tells whether anisotropy is available, and how much.
    /// @throws vk::SystemError If a sampler cannot be created.
    SamplerCache(const core::Device& device, const core::PhysicalDevice& physicalDevice);
    ~SamplerCache() = default;
    SamplerCache(const SamplerCache&) = delete;
    SamplerCache& operator=(const SamplerCache&) = delete;
    SamplerCache(SamplerCache&&) = delete;
    SamplerCache& operator=(SamplerCache&&) = delete;

    /// @return The sampler that reads with @p filter, tiling the texture if @p repeat, clamping to its edges otherwise.
    [[nodiscard]] const vk::raii::Sampler& get(plugins::backend::graphics::TextureFilter filter, bool repeat) const;

  private:
    /// @brief Number of samplers: two filters, two edge modes.
    static constexpr std::size_t kSamplerCount = 4;

    /// @return The index of the sampler for @p filter and @p repeat in _samplers.
    [[nodiscard]] static std::size_t indexOf(plugins::backend::graphics::TextureFilter filter, bool repeat);

    std::array<vk::raii::Sampler, kSamplerCount> _samplers;  ///< Indexed by indexOf().
};

}  // namespace rtype::render::vulkan::texture
