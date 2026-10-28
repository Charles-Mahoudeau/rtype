/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TileDrawing
*/

#include "plugins/tilemap/detail/TileDrawing.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <glm/common.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <numbers>
#include <optional>

#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/Sprite.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/graphics/VisibleArea.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "plugins/tilemap/Tilemap.hpp"

namespace rtype::plugins::tilemap::detail {

namespace {
/// @brief Where a repeated image starts and how many copies of it cover the visible range, along one axis.
struct Span {
    float start = 0.0F;   ///< World coordinate of the first copy.
    float copies = 1.0F;  ///< How many copies, at least one.
};

/// @return The copies of an image at @p position, @p size wide in the world, that cover @p min to @p max.
Span cover(float position, float size, float min, float max) {
    const float start = position + (std::floor((min - position) / size) * size);
    return Span{.start = start, .copies = std::max(1.0F, std::ceil((max - start) / size))};
}
}  // namespace

backend::graphics::Color layerColor(const MapLayer& layer) {
    return layer.tint * backend::graphics::Color{1.0F, 1.0F, 1.0F, layer.opacity};
}

backend::graphics::Sprite tileSprite(const Tilemap& map, const Tileset& tileset, backend::graphics::TextureId texture,
                                     std::uint32_t gid, glm::ivec2 cell, const LayerPlacement& placement,
                                     const backend::graphics::Color& color) {
    const glm::vec2 cellSize{map.getTileSize()};
    const glm::vec2 tileSize{tileset.tileSize};
    const glm::vec2 corner =
        (glm::vec2{cell} * cellSize) + glm::vec2{0.0F, cellSize.y - tileSize.y} + tileset.tileOffset;
    const float horizontal = (gid & kFlipHorizontal) != 0 ? -1.0F : 1.0F;
    const float vertical = (gid & kFlipVertical) != 0 ? -1.0F : 1.0F;
    const bool diagonal = (gid & kFlipDiagonal) != 0;
    const glm::vec2 flip = diagonal ? glm::vec2{vertical, -horizontal} : glm::vec2{horizontal, vertical};
    return backend::graphics::Sprite{.texture = texture,
                                     .source = tileset.getSource((gid & kGidMask) - tileset.firstGid),
                                     .position = placement.position + ((corner + (tileSize / 2.0F)) * placement.scale),
                                     .scale = placement.scale * flip,
                                     .origin = tileSize / 2.0F,
                                     .rotation = diagonal ? std::numbers::pi_v<float> / 2.0F : 0.0F,
                                     .color = color};
}

Bounds chunkBounds(const Tilemap& map, glm::ivec2 origin, const LayerPlacement& placement) {
    const glm::vec2 cellSize{map.getTileSize()};
    const glm::vec2 overhang{map.getOverhang()};
    const glm::vec2 first = (glm::vec2{origin} * cellSize) - overhang;
    const glm::vec2 last = (glm::vec2{origin + glm::ivec2{kChunkSize}} * cellSize) + overhang;
    const glm::vec2 a = placement.position + (first * placement.scale);
    const glm::vec2 b = placement.position + (last * placement.scale);
    return Bounds{.min = glm::min(a, b), .max = glm::max(a, b)};
}

std::optional<backend::graphics::Sprite> imageSprite(const MapLayer& layer, backend::graphics::TextureId texture,
                                                     glm::uvec2 imageSize, const LayerPlacement& placement,
                                                     const backend::graphics::VisibleArea& visible) {
    const glm::vec2 scale = glm::abs(placement.scale);
    const glm::vec2 size = glm::vec2{imageSize} * scale;
    if (visible.isEmpty() || size.x <= 0.0F || size.y <= 0.0F) {
        return std::nullopt;
    }
    Span horizontal{.start = placement.position.x};
    Span vertical{.start = placement.position.y};
    if (layer.repeatX) {
        horizontal = cover(placement.position.x, size.x, visible.min.x, visible.max.x);
    }
    if (layer.repeatY) {
        vertical = cover(placement.position.y, size.y, visible.min.y, visible.max.y);
    }
    const glm::vec2 start{horizontal.start, vertical.start};
    const glm::vec2 copies{horizontal.copies, vertical.copies};
    if (!visible.overlaps(start, start + (size * copies))) {
        return std::nullopt;
    }
    return backend::graphics::Sprite{.texture = texture,
                                     .source = backend::math::Rect{glm::vec2{0.0F}, glm::vec2{imageSize} * copies},
                                     .position = start,
                                     .scale = scale,
                                     .color = layerColor(layer)};
}

}  // namespace rtype::plugins::tilemap::detail
