/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Texture
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_uint2.hpp>
#include <span>

#include "engine/resource/Handle.hpp"
#include "engine/resource/Resource.hpp"

namespace rtype::engine::graphics {

class IRenderer;

struct TextureTag {};

/// @brief Refers to a texture of the renderer without owning it: store it in sprites and ECS components.
using TextureId = resource::Handle<TextureTag>;

/// @brief How a texture is sampled when drawn bigger or smaller than its size.
enum class TextureFilter : std::uint8_t {
    kNearest,  ///< Sharp pixels: pixel art.
    kLinear,   ///< Smooth interpolation.
};

/// @brief Creation parameters of a texture. The pixels are always RGBA, 8 bits per channel.
struct TextureDesc {
    glm::uvec2 size{0};  ///< In pixels.
    TextureFilter filter = TextureFilter::kNearest;
    bool repeat = false;  ///< Tiles the texture when the source rect goes past its edges.
};

/// @brief Owns a texture of the renderer, destroyed when the Texture goes out of scope.
///
/// @details Move-only. To refer to the texture without owning it (Sprite, ECS component), keep its TextureId.
/// The size is kept on this side, so reading it needs no call to the renderer.
class Texture {
  public:
    /// @brief Creates the texture in the renderer.
    /// @param pixels RGBA pixels, 8 bits per channel, row by row from the top: desc.size.x * desc.size.y * 4 bytes.
    Texture(IRenderer& renderer, const TextureDesc& desc, std::span<const std::byte> pixels);
    /// @brief An empty Texture, owning nothing.
    Texture() = default;
    ~Texture() = default;
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) noexcept = default;
    Texture& operator=(Texture&&) noexcept = default;

    /// @return The handle, to refer to the texture without owning it.
    [[nodiscard]] TextureId getId() const noexcept;

    /// @return Size in pixels.
    [[nodiscard]] glm::uvec2 getSize() const noexcept;

  private:
    resource::Resource<TextureTag, IRenderer> _resource;  ///< The owned texture.
    glm::uvec2 _size{0};                                  ///< Size in pixels, from the creation.
};
}  // namespace rtype::engine::graphics
