/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ImageDecoding
*/

#pragma once

#include <cstddef>
#include <filesystem>
#include <glm/ext/vector_uint2.hpp>
#include <span>
#include <string_view>
#include <vector>

namespace rtype::plugins::backend {

/// @brief An image decoded into pixels.
struct DecodedImage {
    glm::uvec2 size{0};             ///< In pixels.
    std::vector<std::byte> pixels;  ///< RGBA, 8 bits per channel, row by row from the top.
};

/// @return The whole content of the file at @p path.
/// @throws exceptions::AssetLoadException If it cannot be read.
[[nodiscard]] std::vector<std::byte> readFile(const std::filesystem::path& path);

/// @brief Decodes an image file's bytes (PNG, JPEG, GIF, BMP, TGA... through stb_image) into RGBA pixels.
/// @param name Names the image in errors.
/// @throws exceptions::AssetLoadException If the bytes are not an image stb_image decodes.
[[nodiscard]] DecodedImage decodeImage(std::string_view name, std::span<const std::byte> encoded);

}  // namespace rtype::plugins::backend
