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
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "plugins/asset/AssetFiles.hpp"
#include "plugins/asset/AssetId.hpp"
#include "plugins/asset/AssetStore.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"
#include "plugins/sprite/detail/AtlasJson.hpp"
#include "plugins/sprite/detail/AtlasPacker.hpp"

namespace rtype::plugins::sprite {

std::size_t AtlasAssets::getCount() const noexcept { return _atlases.getCount(); }

asset::AssetId AtlasAssets::load(const std::filesystem::path& path, const backend::graphics::TextureDesc& desc) {
    const auto stored = _atlases.load(path, [&desc](std::string_view text, const std::filesystem::path& file) {
        return detail::parseAtlasJson(text, file, desc);
    });
    if (stored.added != nullptr) {
        requestTexture(*stored.added);
    }
    return stored.id;
}

asset::AssetId AtlasAssets::add(const std::string& name, SpriteAtlas atlas) {
    const auto stored = _atlases.add(name, std::move(atlas));
    if (stored.added != nullptr) {
        requestTexture(*stored.added);
    }
    return stored.id;
}

asset::AssetId AtlasAssets::pack(const std::string& name, std::span<const std::filesystem::path> images,
                                 std::uint32_t padding, const backend::graphics::TextureDesc& desc) {
    std::string path = asset::normalizePath(name);
    const std::optional<asset::AssetId> id = _atlases.reserve(path);
    if (!id) {
        return asset::AssetId::fromPath(path);
    }
    try {
        detail::PackedImages packed = detail::packImages(images, padding);
        backend::graphics::TextureDesc packedDesc = desc;
        packedDesc.size = packed.size;
        SpriteAtlas atlas{path, packedDesc};
        for (const detail::PackedImages::Placement& placement : packed.placements) {
            atlas.addFrame(placement.name, placement.source);
        }
        requestTexture(_atlases.store(*id, std::move(path), std::move(atlas)), std::move(packed.pixels));
    } catch (const std::exception& error) {
        _atlases.fail(*id, std::move(path), std::format("Atlas '{}': {}", name, error.what()));
    }
    return *id;
}

const SpriteAtlas* AtlasAssets::get(asset::AssetId id) const { return _atlases.get(id); }

const AtlasAssets::Entry* AtlasAssets::getEntry(asset::AssetId id) const { return _atlases.getEntry(id); }

bool AtlasAssets::unload(asset::AssetId id) { return _atlases.unload(id); }

std::vector<AtlasAssets::TextureRequest> AtlasAssets::takeTextureRequests() {
    return std::exchange(_textureRequests, {});
}

void AtlasAssets::requestTexture(const SpriteAtlas& atlas, std::vector<std::byte> pixels) {
    _textureRequests.push_back(
        TextureRequest{.path = atlas.getTexturePath(), .desc = atlas.getTextureDesc(), .pixels = std::move(pixels)});
}

}  // namespace rtype::plugins::sprite
