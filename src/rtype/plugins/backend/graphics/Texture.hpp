/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Texture
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_uint2.hpp>

namespace rtype::plugins::backend::graphics {

/// @brief Refers to a texture of the renderer: store it in sprites and ECS components.
///
/// @details A plain value, cheap to copy, that owns nothing: the renderer creates the texture (createTexture()) and
/// destroys it (destroy(), or its own destruction). The renderer keeps a table indexed by `index`, and bumps the slot's
/// generation each time it frees it, so an id used after its texture was destroyed is detected instead of reaching
/// the slot's next texture. It is local to one renderer: never send it over the network, send an asset path instead.
struct TextureId {
    std::uint32_t index{0};       ///< Slot in the renderer's table.
    std::uint32_t generation{0};  ///< Version of the slot; 0 means "no texture".

    [[nodiscard]] bool operator==(const TextureId& other) const = default;
};

/// @brief How a texture is sampled when drawn bigger or smaller than its size.
enum class TextureFilter : std::uint8_t {
    kNearest,  ///< Sharp pixels: pixel art.
    kLinear,   ///< Smooth interpolation.
};

/// @brief What the pixels of a texture hold.
enum class TextureEncoding : std::uint8_t {
    kSrgb,    ///< sRGB colours with straight alpha: images. The renderer may convert and premultiply them.
    kLinear,  ///< Data sampled as stored, untouched: the distances of a font atlas (Text), normal maps.
};

/// @brief Creation parameters of a texture. The pixels are always RGBA, 8 bits per channel.
struct TextureDesc {
    glm::uvec2 size{0};  ///< In pixels.
    TextureFilter filter = TextureFilter::kNearest;
    bool repeat = false;                                ///< Tiles the texture when the source rect goes past its edges.
    TextureEncoding encoding = TextureEncoding::kSrgb;  ///< Colours or data.
};
}  // namespace rtype::plugins::backend::graphics
