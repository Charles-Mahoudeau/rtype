/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextureAssets
*/

#include "TextureAssets.hpp"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "core/asset/AssetFiles.hpp"
#include "core/asset/AssetId.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "plugins/backend/graphics/IRenderer.hpp"
#include "plugins/backend/graphics/ImageDecoding.hpp"
#include "plugins/backend/graphics/Texture.hpp"

namespace rtype::plugins::backend {
namespace {
/// @brief Bytes of one pixel: RGBA, 8 bits per channel.
constexpr std::size_t kChannels = 4;
}  // namespace

std::size_t TextureAssets::getCount() const noexcept { return _entries.size(); }

core::asset::AssetId TextureAssets::load(const std::filesystem::path& path, const graphics::TextureDesc& desc) {
    return request(core::asset::normalizePath(path), {}, {}, desc);
}

core::asset::AssetId TextureAssets::load(const std::string& name, std::span<const std::byte> encoded,
                                         const graphics::TextureDesc& desc) {
    return request(core::asset::normalizePath(name), {encoded.begin(), encoded.end()}, {}, desc);
}

core::asset::AssetId TextureAssets::loadPixels(const std::string& name, std::span<const std::byte> pixels,
                                               const graphics::TextureDesc& desc) {
    const std::size_t expected = static_cast<std::size_t>(desc.size.x) * desc.size.y * kChannels;
    if (desc.size.x == 0 || desc.size.y == 0 || pixels.size() != expected) {
        throw core::asset::exceptions::AssetLoadException{
            std::format("Texture '{}': {} bytes for {}x{} RGBA pixels", name, pixels.size(), desc.size.x, desc.size.y)};
    }
    return request(core::asset::normalizePath(name), {}, {pixels.begin(), pixels.end()}, desc);
}

const TextureAssets::Entry* TextureAssets::get(core::asset::AssetId id) const {
    const auto found = _entries.find(id);
    return found != _entries.end() ? &found->second : nullptr;
}

std::optional<graphics::TextureId> TextureAssets::getTexture(core::asset::AssetId id) const {
    const Entry* entry = get(id);
    return entry != nullptr && entry->state == AssetState::kLoaded ? std::optional{entry->texture} : std::nullopt;
}

bool TextureAssets::unload(core::asset::AssetId id) {
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

const std::vector<core::asset::AssetId>& TextureAssets::getQueued() const noexcept { return _queued; }

void TextureAssets::setLoaded(core::asset::AssetId id, graphics::TextureId texture, const graphics::TextureDesc& desc) {
    if (Entry* entry = dequeue(id)) {
        entry->state = AssetState::kLoaded;
        entry->texture = texture;
        entry->desc = desc;
        entry->encoded = {};
        entry->pixels = {};
    }
}

void TextureAssets::setFailed(core::asset::AssetId id, std::string error) {
    if (Entry* entry = dequeue(id)) {
        entry->state = AssetState::kFailed;
        entry->error = std::move(error);
        entry->encoded = {};
        entry->pixels = {};
    }
}

std::vector<graphics::TextureId> TextureAssets::takeUnloaded() { return std::exchange(_unloaded, {}); }

core::asset::AssetId TextureAssets::request(std::string path, std::vector<std::byte> encoded,
                                            std::vector<std::byte> pixels, const graphics::TextureDesc& desc) {
    const core::asset::AssetId id = core::asset::AssetId::fromPath(path);
    if (const Entry* known = get(id)) {
        if (known->path != path) {
            throw core::asset::exceptions::AssetLoadException{
                std::format("Texture '{}' has the same id as '{}': rename one of them", path, known->path)};
        }
        return id;
    }
    _entries.emplace(
        id, Entry{.path = std::move(path), .desc = desc, .encoded = std::move(encoded), .pixels = std::move(pixels)});
    _queued.push_back(id);
    return id;
}

TextureAssets::Entry* TextureAssets::dequeue(core::asset::AssetId id) {
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
    const std::vector<core::asset::AssetId> queued = assets.getQueued();
    for (const core::asset::AssetId id : queued) {
        const TextureAssets::Entry& entry = *assets.get(id);
        try {
            graphics::TextureDesc desc = entry.desc;
            if (!entry.pixels.empty()) {
                assets.setLoaded(id, renderer.createTexture(desc, entry.pixels), desc);
                continue;
            }
            const DecodedImage image =
                decodeImage(entry.path, entry.encoded.empty() ? readFile(entry.path) : entry.encoded);
            desc.size = image.size;
            assets.setLoaded(id, renderer.createTexture(desc, image.pixels), desc);
        } catch (const std::exception& error) {
            const std::string message = std::format("Texture {}", error.what());
            std::cerr << "[TextureAssets] " << message << '\n';
            assets.setFailed(id, message);
        }
    }
}
}  // namespace rtype::plugins::backend
