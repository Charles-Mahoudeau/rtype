/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AtlasPacker
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <glm/ext/vector_uint2.hpp>
#include <span>
#include <string>
#include <vector>

#include "plugins/backend/math/Rect.hpp"

namespace rtype::plugins::sprite::detail {

/// @brief Images packed into one: its RGBA pixels and where each image went.
struct PackedImages {
    /// @brief Where one image went.
    struct Placement {
        std::string name;            ///< The image file's stem.
        backend::math::Rect source;  ///< Its rectangle in the packed image, in pixels.
    };

    glm::uvec2 size{0};                 ///< Of the packed image, in pixels.
    std::vector<std::byte> pixels;      ///< RGBA, 8 bits per channel, row by row from the top.
    std::vector<Placement> placements;  ///< One per image, in the order they were given.
};

/// @brief The largest side of a packed image, in pixels: what every GPU accepts.
inline constexpr std::uint32_t kMaxPackedSize = 4096;

/// @brief Decodes the image files @p images (PNG, GIF, BMP... through stb_image) and packs them into the smallest
/// power-of-two image that holds them all (stb_rect_pack), @p padding transparent pixels apart so linear filtering
/// never bleeds one into its neighbour.
/// @throws exceptions::AtlasException If there is no image, one cannot be decoded, two share a stem, or they do not fit
/// in kMaxPackedSize x kMaxPackedSize.
[[nodiscard]] PackedImages packImages(std::span<const std::filesystem::path> images, std::uint32_t padding);

}  // namespace rtype::plugins::sprite::detail
