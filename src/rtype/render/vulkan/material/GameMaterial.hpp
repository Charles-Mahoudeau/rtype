/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GameMaterial
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "plugins/backend/graphics/Shader.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/descriptor/DescriptorAllocator.hpp"
#include "render/vulkan/descriptor/DescriptorSetLayout.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"

namespace rtype::render::vulkan::material {

/// @brief Binding of the material block (a uniform buffer) in the material set, rendering::kMaterialSet.
inline constexpr std::uint32_t kMaterialValuesBinding = 0;
/// @brief Binding of the first extra texture in the material set; the others follow.
inline constexpr std::uint32_t kMaterialFirstTextureBinding = 1;

/// @brief A game material, behind a plugins::backend::graphics::MaterialId: its shader, its values and extra textures,
/// and, per frame in flight, the uniform buffer and the descriptor set (set 2) the GPU reads them from.
///
/// @details update() only records the new values: each frame's copy is brought up to date by prepare(), when the
/// frame is recorded, so a frame the GPU still draws is never written. prepare() also rewrites a frame's set when one
/// of its textures was destroyed since, with the fallback texture in its place (textures are told apart by TextureId,
/// whose generation is never reused, not by their Vulkan handles, which may be).
/// @warning Destroy it before the Allocator, the DescriptorAllocator and the Device.
class RTYPE_RENDER_VULKAN_API GameMaterial {
  public:
    /// @brief What a texture is sampled with, or nothing if it no longer exists.
    struct TextureBinding {
        vk::ImageView view;   ///< Its image.
        vk::Sampler sampler;  ///< Its filter and repeat mode.
    };
    /// @brief Finds how to sample a texture, or nothing if it no longer exists.
    using TextureFinder = std::function<std::optional<TextureBinding>(plugins::backend::graphics::TextureId)>;

    /// @param materialSize The bytes of its shader's material block: @p desc.values must be that many.
    /// @throws vk::SystemError If a buffer or a set cannot be created.
    GameMaterial(const memory::Allocator& allocator, descriptor::DescriptorAllocator& descriptors,
                 const descriptor::DescriptorSetLayout& layout, std::uint32_t materialSize,
                 const plugins::backend::graphics::MaterialDesc& desc, std::size_t framesInFlight);
    ~GameMaterial() = default;
    GameMaterial(const GameMaterial&) = delete;
    GameMaterial& operator=(const GameMaterial&) = delete;
    GameMaterial(GameMaterial&&) noexcept = default;
    GameMaterial& operator=(GameMaterial&&) noexcept = default;

    /// @return Its shader.
    [[nodiscard]] plugins::backend::graphics::ShaderId getShader() const noexcept { return _shader; }
    /// @return Its textures, by slot.
    [[nodiscard]] const std::array<plugins::backend::graphics::TextureId,
                                   plugins::backend::graphics::kShaderMaxTextures>&
    getTextures() const noexcept {
        return _textures;
    }

    /// @brief Records @p desc's values and textures (its shader is ignored), for the next frames prepare()d.
    void update(const plugins::backend::graphics::MaterialDesc& desc);

    /// @return Frame @p frameIndex's set, brought up to date first: its values written, and its textures bound (an
    /// unset or destroyed one as @p fallback, which must exist).
    /// @warning Only while recording frame @p frameIndex, after its fence was waited for.
    [[nodiscard]] vk::DescriptorSet prepare(const core::Device& device, std::size_t frameIndex,
                                            const TextureFinder& findTexture,
                                            plugins::backend::graphics::TextureId fallback);

  private:
    /// @brief One frame's copy.
    struct Frame {
        std::optional<memory::Buffer> values;  ///< The material block; none if it is empty.
        vk::raii::DescriptorSet set;           ///< Set 2: the block and the textures.
        std::uint64_t version{0};              ///< The version of the values it holds; 0: never written.
        std::array<plugins::backend::graphics::TextureId, plugins::backend::graphics::kShaderMaxTextures>
            bound{};  ///< The textures its bindings point to; none yet if default.
    };

    plugins::backend::graphics::ShaderId _shader;  ///< Its shader.
    std::vector<std::byte> _values;                ///< The latest material block.
    std::array<plugins::backend::graphics::TextureId, plugins::backend::graphics::kShaderMaxTextures>
        _textures{};             ///< The latest textures, by slot.
    std::uint64_t _version{1};   ///< Incremented by update().
    std::vector<Frame> _frames;  ///< One per frame in flight.
};

}  // namespace rtype::render::vulkan::material
