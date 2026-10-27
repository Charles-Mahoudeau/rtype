/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TilemapAssets
*/

#include "plugins/tilemap/TilemapAssets.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "plugins/asset/AssetFiles.hpp"
#include "plugins/asset/AssetId.hpp"
#include "plugins/asset/AssetStore.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/tilemap/Tilemap.hpp"
#include "plugins/tilemap/detail/TiledJson.hpp"

namespace rtype::plugins::tilemap {

std::size_t TilemapAssets::getCount() const noexcept { return _maps.getCount(); }

asset::AssetId TilemapAssets::load(const std::filesystem::path& path) {
    const auto stored = _maps.load(path, [](std::string_view text, const std::filesystem::path& file) {
        return detail::parseTiledMap(text, file, &asset::readTextFile);
    });
    if (stored.added != nullptr) {
        requestTextures(*stored.added);
    }
    return stored.id;
}

asset::AssetId TilemapAssets::add(const std::string& name, Tilemap map) {
    const auto stored = _maps.add(name, std::move(map));
    if (stored.added != nullptr) {
        requestTextures(*stored.added);
    }
    return stored.id;
}

const Tilemap* TilemapAssets::get(asset::AssetId id) const { return _maps.get(id); }

const TilemapAssets::Entry* TilemapAssets::getEntry(asset::AssetId id) const { return _maps.getEntry(id); }

bool TilemapAssets::unload(asset::AssetId id) { return _maps.unload(id); }

std::vector<TilemapAssets::TextureRequest> TilemapAssets::takeTextureRequests() {
    return std::exchange(_textureRequests, {});
}

void TilemapAssets::requestTextures(const Tilemap& map) {
    const auto request = [this](const std::string& image, bool repeat) {
        const auto known = std::ranges::find(_textureRequests, image, &TextureRequest::path);
        if (known == _textureRequests.end()) {
            _textureRequests.push_back(TextureRequest{.path = image, .desc = {.repeat = repeat}});
        } else {
            known->desc.repeat = known->desc.repeat || repeat;
        }
    };
    for (const Tileset& tileset : map.getTilesets()) {
        request(tileset.imagePath, false);
    }
    for (const MapLayer& layer : map.getLayers()) {
        if (layer.kind == LayerKind::kImage) {
            request(layer.imagePath, layer.repeatX || layer.repeatY);
        }
    }
}

}  // namespace rtype::plugins::tilemap
