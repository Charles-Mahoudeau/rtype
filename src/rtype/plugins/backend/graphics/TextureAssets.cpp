/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextureAssets
*/

#include "TextureAssets.hpp"

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
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "plugins/backend/exceptions/AssetExceptions.hpp"
#include "plugins/backend/graphics/IRenderer.hpp"
#include "plugins/backend/graphics/Texture.hpp"

namespace rtype::plugins::backend {
namespace {
/// @brief Bytes of one decoded pixel: RGBA, 8 bits per channel.
constexpr int kChannels = 4;

/// @brief Frees the pixels stb_image decoded.
struct StbFree {
    void operator()(stbi_uc* pixels) const noexcept { stbi_image_free(pixels); }
};

/// @return The whole content of a file.
/// @throws exceptions::AssetLoadException If it cannot be read.
std::vector<std::byte> readFile(const std::filesystem::path& path) {
    std::ifstream file{path, std::ios::binary};
    if (!file) {
        throw exceptions::AssetLoadException{std::format("Texture '{}': cannot open the file", path.generic_string())};
    }
    const std::vector<char> content{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    std::vector<std::byte> bytes(content.size());
    std::ranges::transform(content, bytes.begin(), [](char byte) { return static_cast<std::byte>(byte); });
    return bytes;
}
}  // namespace

graphics::TextureId loadTexture(TextureAssets& assets, graphics::IRenderer& renderer, const std::filesystem::path& path,
                                const TextureOptions& options) {
    const std::string name = path.generic_string();
    if (const std::optional<graphics::TextureId> loaded = assets.find(name)) {
        return *loaded;
    }
    return loadTexture(assets, renderer, name, readFile(path), options);
}

graphics::TextureId loadTexture(TextureAssets& assets, graphics::IRenderer& renderer, const std::string& name,
                                std::span<const std::byte> encoded, const TextureOptions& options) {
    if (const std::optional<graphics::TextureId> loaded = assets.find(name)) {
        return *loaded;
    }
    int width = 0;
    int height = 0;
    int fileChannels = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const std::unique_ptr<stbi_uc, StbFree> pixels{
        stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(encoded.data()), static_cast<int>(encoded.size()),
                              &width, &height, &fileChannels, kChannels)};
    if (pixels == nullptr) {
        throw exceptions::AssetLoadException{
            std::format("Texture '{}': cannot decode the image ({})", name, stbi_failure_reason())};
    }
    const graphics::TextureDesc desc{.size = {static_cast<unsigned>(width), static_cast<unsigned>(height)},
                                     .filter = options.filter,
                                     .repeat = options.repeat};
    const std::size_t byteCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * kChannels;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const std::span<const std::byte> rgba{reinterpret_cast<const std::byte*>(pixels.get()), byteCount};
    const graphics::TextureId id = renderer.createTexture(desc, rgba);
    if (!assets.add(name, id, TextureInfo{.size = desc.size})) {
        throw exceptions::AssetLoadException{
            std::format("Texture '{}': the renderer returned an id already in use", name)};
    }
    return id;
}
}  // namespace rtype::plugins::backend
