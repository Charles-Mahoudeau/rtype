/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TilemapAssets
*/

#include "plugins/tilemap/TilemapAssets.hpp"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "plugins/asset/AssetId.hpp"
#include "plugins/backend/exceptions/AssetExceptions.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/tilemap/Tilemap.hpp"
#include "plugins/tilemap/detail/TiledJson.hpp"
#include "plugins/tilemap/exceptions/TilemapExceptions.hpp"

namespace rtype::plugins::tilemap {

namespace {
/// @return @p path, normalized as AssetId::fromPath() sees it, with forward slashes.
std::string normalize(const std::filesystem::path& path) { return path.lexically_normal().generic_string(); }

/// @return The whole content of the text file at @p path.
/// @throws exceptions::TilemapException If it cannot be read.
std::string readText(const std::filesystem::path& path) {
    std::ifstream file{path, std::ios::binary};
    if (!file) {
        throw exceptions::TilemapException{std::format("cannot open '{}'", path.generic_string())};
    }
    return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}
}  // namespace

std::size_t TilemapAssets::getCount() const noexcept { return _entries.size(); }

asset::AssetId TilemapAssets::load(const std::filesystem::path& path) {
    std::string name = normalize(path);
    const std::optional<asset::AssetId> id = reserve(name);
    if (!id) {
        return asset::AssetId::fromPath(name);
    }
    try {
        store(*id, std::move(name), detail::parseTiledMap(readText(path), path, &readText));
    } catch (const std::exception& error) {
        fail(*id, std::move(name), error.what());
    }
    return *id;
}

asset::AssetId TilemapAssets::add(const std::string& name, Tilemap map) {
    std::string path = normalize(name);
    const std::optional<asset::AssetId> id = reserve(path);
    if (!id) {
        return asset::AssetId::fromPath(path);
    }
    store(*id, std::move(path), std::move(map));
    return *id;
}

const Tilemap* TilemapAssets::get(asset::AssetId id) const {
    const Entry* entry = getEntry(id);
    return entry != nullptr && entry->map ? &*entry->map : nullptr;
}

const TilemapAssets::Entry* TilemapAssets::getEntry(asset::AssetId id) const {
    const auto found = _entries.find(id);
    return found != _entries.end() ? &found->second : nullptr;
}

bool TilemapAssets::unload(asset::AssetId id) { return _entries.erase(id) != 0; }

std::vector<TilemapAssets::TextureRequest> TilemapAssets::takeTextureRequests() {
    return std::exchange(_textureRequests, {});
}

std::optional<asset::AssetId> TilemapAssets::reserve(const std::string& path) const {
    const asset::AssetId id = asset::AssetId::fromPath(path);
    if (const Entry* known = getEntry(id)) {
        if (known->path != path) {
            throw backend::exceptions::AssetLoadException{
                std::format("Tilemap '{}' has the same id as '{}': rename one of them", path, known->path)};
        }
        return std::nullopt;
    }
    return id;
}

void TilemapAssets::store(asset::AssetId id, std::string path, Tilemap map) {
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
    _entries.emplace(id, Entry{.path = std::move(path), .map = std::move(map)});
}

void TilemapAssets::fail(asset::AssetId id, std::string path, std::string error) {
    std::cerr << "[TilemapAssets] " << error << '\n';
    _entries.emplace(id, Entry{.path = std::move(path), .error = std::move(error)});
}

}  // namespace rtype::plugins::tilemap
