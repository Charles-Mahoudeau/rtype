/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Tilemap
*/

#include "plugins/tilemap/Tilemap.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <glm/common.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <iterator>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "plugins/backend/math/Rect.hpp"

namespace rtype::plugins::tilemap {

namespace {
/// @return The chunk origin of @p cell: its coordinates rounded down to a multiple of kChunkSize.
glm::ivec2 chunkOriginOf(glm::ivec2 cell) {
    const auto down = [](std::int32_t value) {
        return (value >= 0 ? value : value - (kChunkSize - 1)) / kChunkSize * kChunkSize;
    };
    return {down(cell.x), down(cell.y)};
}

/// @return The index of @p cell in the gids of the chunk at @p origin.
std::size_t indexIn(glm::ivec2 origin, glm::ivec2 cell) {
    return (static_cast<std::size_t>(cell.y - origin.y) * kChunkSize) + static_cast<std::size_t>(cell.x - origin.x);
}

/// @return The chunk of @p layer at @p origin, or nullptr.
const TileChunk* findChunk(const MapLayer& layer, glm::ivec2 origin) {
    const auto found = std::ranges::find(layer.chunks, origin, &TileChunk::origin);
    return found != layer.chunks.end() ? &*found : nullptr;
}
}  // namespace

backend::math::Rect Tileset::getSource(std::uint32_t localId) const {
    const std::uint32_t perRow = std::max(columns, 1U);
    const glm::uvec2 cell{localId % perRow, localId / perRow};
    const glm::uvec2 corner = glm::uvec2{margin} + (cell * (tileSize + glm::uvec2{spacing}));
    return backend::math::Rect{glm::vec2{corner}, glm::vec2{tileSize}};
}

std::uint32_t MapLayer::getTile(glm::ivec2 cell) const {
    const glm::ivec2 origin = chunkOriginOf(cell);
    const TileChunk* chunk = findChunk(*this, origin);
    return chunk != nullptr ? chunk->gids.at(indexIn(origin, cell)) : 0;
}

void setTiles(MapLayer& layer, glm::ivec2 origin, std::uint32_t width, std::span<const std::uint32_t> gids) {
    if (width == 0) {
        return;
    }
    std::size_t index = 0;
    for (const std::uint32_t gid : gids) {
        const glm::ivec2 cell =
            origin + glm::ivec2{static_cast<std::int32_t>(index % width), static_cast<std::int32_t>(index / width)};
        ++index;
        const glm::ivec2 chunkOrigin = chunkOriginOf(cell);
        auto chunk = std::ranges::find(layer.chunks, chunkOrigin, &TileChunk::origin);
        if (chunk == layer.chunks.end()) {
            if (gid == 0) {
                continue;
            }
            layer.chunks.push_back(TileChunk{.origin = chunkOrigin});
            chunk = std::prev(layer.chunks.end());
        }
        std::uint32_t& slot = chunk->gids.at(indexIn(chunkOrigin, cell));
        chunk->count -= slot != 0 ? 1 : 0;
        chunk->count += gid != 0 ? 1 : 0;
        slot = gid;
    }
    std::erase_if(layer.chunks, [](const TileChunk& chunk) { return chunk.count == 0; });
}

Tilemap::Tilemap(glm::uvec2 tileSize, std::vector<Tileset> tilesets, std::vector<MapLayer> layers,
                 glm::vec2 parallaxOrigin)
    : _tileSize{tileSize}, _tilesets{std::move(tilesets)}, _layers{std::move(layers)}, _parallaxOrigin{parallaxOrigin} {
    std::ranges::sort(_tilesets, {}, &Tileset::firstGid);
}

std::optional<std::uint32_t> Tilemap::findLayer(std::string_view name) const {
    const auto found = std::ranges::find(_layers, name, &MapLayer::name);
    if (found == _layers.end()) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(std::distance(_layers.begin(), found));
}

const Tileset* Tilemap::findTileset(std::uint32_t gid) const {
    const std::uint32_t id = gid & kGidMask;
    const Tileset* found = nullptr;
    for (const Tileset& tileset : _tilesets) {
        if (tileset.firstGid > id) {
            break;
        }
        found = &tileset;
    }
    return found != nullptr && id < found->firstGid + found->tileCount ? found : nullptr;
}

float Tilemap::getOverhang() const noexcept {
    float overhang = 0.0F;
    for (const Tileset& tileset : _tilesets) {
        const glm::vec2 beyond = glm::max(glm::vec2{tileset.tileSize} - glm::vec2{_tileSize}, glm::vec2{0.0F});
        const glm::vec2 offset = glm::abs(tileset.tileOffset);
        overhang = std::max({overhang, beyond.x + offset.x, beyond.y + offset.y});
    }
    return overhang;
}

}  // namespace rtype::plugins::tilemap
