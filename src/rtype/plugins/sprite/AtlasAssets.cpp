/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AtlasAssets
*/

#include "plugins/sprite/AtlasAssets.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/backend/exceptions/AssetExceptions.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"
#include "plugins/sprite/detail/AtlasJson.hpp"
#include "plugins/sprite/detail/AtlasPacker.hpp"
#include "plugins/sprite/exceptions/SpriteExceptions.hpp"

namespace rtype::plugins::sprite {

namespace {
/// @return @p path, normalized as AssetId::fromPath() sees it, with forward slashes.
std::string normalize(const std::filesystem::path& path) { return path.lexically_normal().generic_string(); }

/// @return The whole content of the text file at @p path.
/// @throws exceptions::AtlasException If it cannot be read.
std::string readText(const std::filesystem::path& path) {
    std::ifstream file{path, std::ios::binary};
    if (!file) {
        throw exceptions::AtlasException{std::format("Atlas '{}': cannot open the file", path.generic_string())};
    }
    return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}
}  // namespace

std::size_t AtlasAssets::getCount() const noexcept { return _entries.size(); }

core::asset::AssetId AtlasAssets::load(const std::filesystem::path& path, const backend::graphics::TextureDesc& desc) {
    std::string name = normalize(path);
    const std::optional<core::asset::AssetId> id = reserve(name);
    if (!id) {
        return core::asset::AssetId::fromPath(name);
    }
    try {
        store(*id, std::move(name), detail::parseAtlasJson(readText(path), path, desc));
    } catch (const std::exception& error) {
        fail(*id, std::move(name), error.what());
    }
    return *id;
}

core::asset::AssetId AtlasAssets::add(const std::string& name, SpriteAtlas atlas) {
    std::string path = normalize(name);
    const std::optional<core::asset::AssetId> id = reserve(path);
    if (!id) {
        return core::asset::AssetId::fromPath(path);
    }
    store(*id, std::move(path), std::move(atlas));
    return *id;
}

core::asset::AssetId AtlasAssets::pack(const std::string& name, std::span<const std::filesystem::path> images,
                                       std::uint32_t padding, const backend::graphics::TextureDesc& desc) {
    std::string path = normalize(name);
    const std::optional<core::asset::AssetId> id = reserve(path);
    if (!id) {
        return core::asset::AssetId::fromPath(path);
    }
    try {
        detail::PackedImages packed = detail::packImages(images, padding);
        backend::graphics::TextureDesc packedDesc = desc;
        packedDesc.size = packed.size;
        SpriteAtlas atlas{path, packedDesc};
        for (const detail::PackedImages::Placement& placement : packed.placements) {
            atlas.addFrame(placement.name, placement.source);
        }
        store(*id, std::move(path), std::move(atlas), std::move(packed.pixels));
    } catch (const std::exception& error) {
        fail(*id, std::move(path), std::format("Atlas '{}': {}", name, error.what()));
    }
    return *id;
}

const SpriteAtlas* AtlasAssets::get(core::asset::AssetId id) const {
    const Entry* entry = getEntry(id);
    return entry != nullptr && entry->atlas ? &*entry->atlas : nullptr;
}

const AtlasAssets::Entry* AtlasAssets::getEntry(core::asset::AssetId id) const {
    const auto found = _entries.find(id);
    return found != _entries.end() ? &found->second : nullptr;
}

bool AtlasAssets::unload(core::asset::AssetId id) { return _entries.erase(id) != 0; }

std::vector<AtlasAssets::TextureRequest> AtlasAssets::takeTextureRequests() {
    return std::exchange(_textureRequests, {});
}

std::optional<core::asset::AssetId> AtlasAssets::reserve(const std::string& path) const {
    const core::asset::AssetId id = core::asset::AssetId::fromPath(path);
    if (const Entry* known = getEntry(id)) {
        if (known->path != path) {
            throw backend::exceptions::AssetLoadException{
                std::format("Atlas '{}' has the same id as '{}': rename one of them", path, known->path)};
        }
        return std::nullopt;
    }
    return id;
}

void AtlasAssets::store(core::asset::AssetId id, std::string path, SpriteAtlas atlas, std::vector<std::byte> pixels) {
    _textureRequests.push_back(
        TextureRequest{.path = atlas.getTexturePath(), .desc = atlas.getTextureDesc(), .pixels = std::move(pixels)});
    _entries.emplace(id, Entry{.path = std::move(path), .atlas = std::move(atlas)});
}

void AtlasAssets::fail(core::asset::AssetId id, std::string path, std::string error) {
    std::cerr << "[AtlasAssets] " << error << '\n';
    _entries.emplace(id, Entry{.path = std::move(path), .error = std::move(error)});
}

}  // namespace rtype::plugins::sprite
