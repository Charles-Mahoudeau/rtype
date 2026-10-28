/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteBatch
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "plugins/backend/graphics/Sprite.hpp"
#include "plugins/backend/graphics/Text.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "render/vulkan/Export.hpp"

namespace rtype::render::vulkan::rendering {

/// @brief One sprite in the per-frame instance buffer, read by shaders/sprite.vert as per-instance vertex attributes:
/// 64 bytes. The vertex shader turns a unit quad into the sprite: `position + rotate(corner * size - origin)`.
struct SpriteInstance {
    glm::vec2 position{0.0F};  ///< World point the origin sits on. Location 0.
    glm::vec2 size{0.0F};      ///< In world units: source size times scale. Location 1.
    glm::vec2 origin{0.0F};    ///< Pivot from the quad's top-left corner, in world units. Location 2.
    float rotation = 0.0F;     ///< In radians, clockwise on screen. Location 3.
    std::uint32_t blend = 0;  ///< graphics::BlendMode and the MSDF flag, as rendering/Blend.hpp packs them. Location 6.
    glm::vec4 uvRect{0.0F, 0.0F, 1.0F, 1.0F};  ///< Part of the texture: (u0, v0, u1, v1), 0 to 1. Location 4.
    glm::vec4 color{1.0F};                     ///< Multiplies the texture. Location 5.

    /// @return The instance of @p sprite, drawn from a texture of @p textureSize pixels.
    [[nodiscard]] RTYPE_RENDER_VULKAN_API static SpriteInstance fromSprite(
        const plugins::backend::graphics::Sprite& sprite, glm::uvec2 textureSize) noexcept;

    /// @return The instance of @p glyph of @p text, drawn from its MSDF font atlas of @p textureSize pixels: placed by
    /// the text's position, scale, rotation and origin, read as distances by the sprite shaders (kShaderMsdf).
    [[nodiscard]] RTYPE_RENDER_VULKAN_API static SpriteInstance fromGlyph(
        const plugins::backend::graphics::Text& text, const plugins::backend::graphics::GlyphQuad& glyph,
        glm::uvec2 textureSize) noexcept;

    /// @return The binding of the instance buffer: one SpriteInstance per instance.
    [[nodiscard]] RTYPE_RENDER_VULKAN_API static std::array<vk::VertexInputBindingDescription, 1> getBindings(
        std::uint32_t binding = 0) noexcept;
    /// @return The attributes, locations 0 to 6.
    [[nodiscard]] RTYPE_RENDER_VULKAN_API static std::array<vk::VertexInputAttributeDescription, 7> getAttributes(
        std::uint32_t binding = 0) noexcept;
};
static_assert(sizeof(SpriteInstance) == 64 && offsetof(SpriteInstance, blend) == 28 &&
                  offsetof(SpriteInstance, uvRect) == 32 && offsetof(SpriteInstance, color) == 48,
              "SpriteInstance must stay 64 tightly packed bytes: its attribute offsets assume it");

/// @brief The sprites of a frame, in draw order, grouped in batches: consecutive sprites of the same texture are drawn
/// by one instanced draw call.
///
/// @details Only consecutive sprites are merged, so the draw order (the last drawn on top) is kept: sort the sprites
/// by layer, then by texture within a layer, to get fewer batches.
class RTYPE_RENDER_VULKAN_API SpriteBatch {
  public:
    /// @brief Consecutive instances sharing a texture.
    struct Batch {
        plugins::backend::graphics::TextureId texture;  ///< Bound for the whole batch.
        std::uint32_t firstInstance = 0;                ///< Index of its first instance in getInstances().
        std::uint32_t instanceCount = 0;                ///< Number of instances.
    };

    SpriteBatch() = default;
    ~SpriteBatch() = default;
    SpriteBatch(const SpriteBatch&) = default;
    SpriteBatch& operator=(const SpriteBatch&) = default;
    SpriteBatch(SpriteBatch&&) noexcept = default;
    SpriteBatch& operator=(SpriteBatch&&) noexcept = default;

    /// @return The instances added since clear(), in draw order.
    [[nodiscard]] std::span<const SpriteInstance> getInstances() const noexcept { return _instances; }
    /// @return The batches, in draw order.
    [[nodiscard]] std::span<const Batch> getBatches() const noexcept { return _batches; }

    /// @brief Adds @p instance, drawn with @p texture, after the others: it extends the last batch if it shares its
    /// texture, or starts a new one.
    void add(plugins::backend::graphics::TextureId texture, const SpriteInstance& instance);

    /// @brief Forgets every instance and batch, keeping the memory for the next frame.
    void clear() noexcept;

  private:
    std::vector<SpriteInstance> _instances;  ///< In draw order.
    std::vector<Batch> _batches;             ///< In draw order.
};

}  // namespace rtype::render::vulkan::rendering
