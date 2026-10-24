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
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "plugins/asset/AssetId.hpp"
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

/// @return The RGBA pixels of the encoded image @p encoded, and @p desc completed with its size.
/// @throws exceptions::AssetLoadException If the bytes are not an image stb_image decodes.
std::vector<std::byte> decodeImage(const std::string& name, std::span<const std::byte> encoded,
                                   graphics::TextureDesc& desc) {
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
            std::format("Texture '{}': cannot decode the image ({})", name, stbi_failure_reason())};
    }
    desc.size = {static_cast<unsigned>(width), static_cast<unsigned>(height)};
    const std::size_t byteCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * kChannels;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const std::span<const std::byte> rgba{reinterpret_cast<const std::byte*>(pixels.get()), byteCount};
    return {rgba.begin(), rgba.end()};
}
}  // namespace

std::size_t TextureAssets::getCount() const noexcept { return _entries.size(); }

asset::AssetId TextureAssets::load(const std::filesystem::path& path, const graphics::TextureDesc& desc) {
    return request(path.lexically_normal().generic_string(), {}, desc);
}

asset::AssetId TextureAssets::load(const std::string& name, std::span<const std::byte> encoded,
                                   const graphics::TextureDesc& desc) {
    return request(std::filesystem::path{name}.lexically_normal().generic_string(), {encoded.begin(), encoded.end()},
                   desc);
}

const TextureAssets::Entry* TextureAssets::get(asset::AssetId id) const {
    const auto found = _entries.find(id);
    return found != _entries.end() ? &found->second : nullptr;
}

std::optional<graphics::TextureId> TextureAssets::getTexture(asset::AssetId id) const {
    const Entry* entry = get(id);
    return entry != nullptr && entry->state == AssetState::kLoaded ? std::optional{entry->texture} : std::nullopt;
}

bool TextureAssets::unload(asset::AssetId id) {
    const auto found = _entries.find(id);
    if (found == _entries.end()) {
        return false;
    }
    if (found->second.state == AssetState::kLoaded) {
        _unloaded.push_back(found->second.texture);
    }
    std::erase(_queued, id);
    _entries.erase(found);
    return true;
}

const std::vector<asset::AssetId>& TextureAssets::getQueued() const noexcept { return _queued; }

void TextureAssets::setLoaded(asset::AssetId id, graphics::TextureId texture, const graphics::TextureDesc& desc) {
    if (Entry* entry = dequeue(id)) {
        entry->state = AssetState::kLoaded;
        entry->texture = texture;
        entry->desc = desc;
        entry->encoded = {};
    }
}

void TextureAssets::setFailed(asset::AssetId id, std::string error) {
    if (Entry* entry = dequeue(id)) {
        entry->state = AssetState::kFailed;
        entry->error = std::move(error);
        entry->encoded = {};
    }
}

std::vector<graphics::TextureId> TextureAssets::takeUnloaded() { return std::exchange(_unloaded, {}); }

asset::AssetId TextureAssets::request(std::string path, std::vector<std::byte> encoded,
                                      const graphics::TextureDesc& desc) {
    const asset::AssetId id = asset::AssetId::fromPath(path);
    if (const Entry* known = get(id)) {
        if (known->path != path) {
            throw exceptions::AssetLoadException{
                std::format("Texture '{}' has the same id as '{}': rename one of them", path, known->path)};
        }
        return id;
    }
    _entries.emplace(id, Entry{.path = std::move(path), .desc = desc, .encoded = std::move(encoded)});
    _queued.push_back(id);
    return id;
}

TextureAssets::Entry* TextureAssets::dequeue(asset::AssetId id) {
    const auto queued = std::ranges::find(_queued, id);
    if (queued == _queued.end()) {
        return nullptr;
    }
    _queued.erase(queued);
    return &_entries.at(id);
}

void updateTextures(TextureAssets& assets, graphics::IRenderer& renderer) {
    for (const graphics::TextureId texture : assets.takeUnloaded()) {
        renderer.destroy(texture);
    }
    const std::vector<asset::AssetId> queued = assets.getQueued();
    for (const asset::AssetId id : queued) {
        const TextureAssets::Entry& entry = *assets.get(id);
        try {
            graphics::TextureDesc desc = entry.desc;
            const std::vector<std::byte> pixels = entry.encoded.empty()
                                                      ? decodeImage(entry.path, readFile(entry.path), desc)
                                                      : decodeImage(entry.path, entry.encoded, desc);
            assets.setLoaded(id, renderer.createTexture(desc, pixels), desc);
        } catch (const std::exception& error) {
            std::cerr << "[TextureAssets] " << error.what() << '\n';
            assets.setFailed(id, error.what());
        }
    }
}
}  // namespace rtype::plugins::backend
