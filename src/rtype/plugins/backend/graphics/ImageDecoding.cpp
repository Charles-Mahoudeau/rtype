/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ImageDecoding
*/

#include "ImageDecoding.hpp"

// stb_image's implementation, compiled in the only file that uses it. Static, so it cannot clash with another module
// that compiles its own copy.
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "plugins/backend/exceptions/AssetExceptions.hpp"

namespace rtype::plugins::backend {

namespace {
/// @brief Bytes of one decoded pixel: RGBA, 8 bits per channel.
constexpr int kChannels = 4;

/// @brief Frees the pixels stb_image decoded.
struct StbFree {
    void operator()(stbi_uc* pixels) const noexcept { stbi_image_free(pixels); }
};
}  // namespace

std::vector<std::byte> readFile(const std::filesystem::path& path) {
    std::ifstream file{path, std::ios::binary};
    if (!file) {
        throw exceptions::AssetLoadException{std::format("'{}': cannot open the file", path.generic_string())};
    }
    const std::vector<char> content{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    std::vector<std::byte> bytes(content.size());
    std::ranges::transform(content, bytes.begin(), [](char byte) { return static_cast<std::byte>(byte); });
    return bytes;
}

DecodedImage decodeImage(std::string_view name, std::span<const std::byte> encoded) {
    int width = 0;
    int height = 0;
    int fileChannels = 0;
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast): stb_image reads unsigned bytes.
    const std::unique_ptr<stbi_uc, StbFree> pixels{
        stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(encoded.data()), static_cast<int>(encoded.size()),
                              &width, &height, &fileChannels, kChannels)};
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    if (pixels == nullptr) {
        throw exceptions::AssetLoadException{
            std::format("'{}': cannot decode the image ({})", name, stbi_failure_reason())};
    }
    const std::size_t byteCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * kChannels;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const std::span<const std::byte> rgba{reinterpret_cast<const std::byte*>(pixels.get()), byteCount};
    return DecodedImage{.size = {static_cast<unsigned>(width), static_cast<unsigned>(height)},
                        .pixels = {rgba.begin(), rgba.end()}};
}

}  // namespace rtype::plugins::backend
