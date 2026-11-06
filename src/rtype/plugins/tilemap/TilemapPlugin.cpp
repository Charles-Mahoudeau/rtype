/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TilemapPlugin
*/

#include "plugins/tilemap/TilemapPlugin.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>
#include <iostream>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "plugins/graphics/Camera.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/SpriteQueue.hpp"
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/graphics/VisibleArea.hpp"
#include "plugins/tilemap/Parallax.hpp"
#include "plugins/tilemap/Tilemap.hpp"
#include "plugins/tilemap/TilemapAssets.hpp"
#include "plugins/tilemap/TilemapLayer.hpp"
#include "plugins/tilemap/detail/TileDrawing.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/contract/renderer/drawing/Color.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/time/Time.hpp"

namespace rtype::plugins::tilemap {

namespace {
/// @brief Render, client: moves each Parallax entity by the camera and its scroll.
void applyParallax(ecs::Res<ecs::Time> time,
                   ecs::Query<const transform::GlobalTransform, const graphics::Camera, ecs::Without<Parallax>> cameras,
                   ecs::Query<Parallax, transform::GlobalTransform> moved) {
    std::optional<glm::vec2> camera;
    std::int32_t priority = 0;
    for (auto [entity, global, shown] : cameras) {
        if (!camera || shown.priority > priority) {
            camera = global.position;
            priority = shown.priority;
        }
    }
    for (auto [entity, parallax, global] : moved) {
        if (parallax.get().scroll != glm::vec2{0.0F}) {
            parallax->scrolled += parallax.get().scroll * time->delta;
        }
        const Parallax& current = parallax.get();
        const glm::vec2 follow =
            camera ? (*camera - current.origin) * (glm::vec2{1.0F} - current.factor) : glm::vec2{0.0F};
        const glm::vec2 shift = follow + current.scrolled;
        if (shift != glm::vec2{0.0F}) {
            global->position += shift;
        }
    }
}

/// @brief Render, client: hands the maps' images to TextureAssets.
void queueMapTextures(ecs::ResMut<TilemapAssets> maps, ecs::ResAppend<graphics::TextureAssets> textures) {
    for (const TilemapAssets::TextureRequest& request : maps->takeTextureRequests()) {
        try {
            static_cast<void>(textures->load(request.path, request.desc));
        } catch (const std::exception& error) {
            std::cerr << "[TilemapAssets] " << error.what() << '\n';
        }
    }
}

/// @brief Pushes the visible tiles of the tile layer @p layer of @p map.
void drawTiles(const Tilemap& map, const MapLayer& layer, std::int32_t drawLayer,
               const detail::LayerPlacement& placement, const graphics::TextureAssets& textures,
               const graphics::VisibleArea& visible, graphics::SpriteQueue& queue) {
    const std::vector<Tileset>& tilesets = map.getTilesets();
    std::vector<std::optional<contract::renderer::TextureId>> loaded;
    loaded.reserve(tilesets.size());
    for (const Tileset& tileset : tilesets) {
        loaded.push_back(textures.getTexture(tileset.texture));
    }
    const contract::renderer::Color color = detail::layerColor(layer);
    for (const TileChunk& chunk : layer.chunks) {
        const detail::Bounds bounds = detail::chunkBounds(map, chunk.origin, placement);
        if (!visible.overlaps(bounds.min, bounds.max)) {
            continue;
        }
        std::size_t index = 0;
        for (const std::uint32_t gid : chunk.gids) {
            const glm::ivec2 cell = chunk.origin + glm::ivec2{static_cast<std::int32_t>(index % kChunkSize),
                                                              static_cast<std::int32_t>(index / kChunkSize)};
            ++index;
            const Tileset* tileset = gid != 0 ? map.findTileset(gid) : nullptr;
            if (tileset == nullptr) {
                continue;
            }
            const std::optional<contract::renderer::TextureId>& texture =
                loaded.at(static_cast<std::size_t>(std::distance(tilesets.data(), tileset)));
            if (texture) {
                queue.push(drawLayer, detail::tileSprite(map, *tileset, *texture, gid, cell, placement, color));
            }
        }
    }
}

/// @brief Pushes the image of the image layer @p layer, if it is loaded and visible.
void drawImage(const MapLayer& layer, std::int32_t drawLayer, const detail::LayerPlacement& placement,
               const graphics::TextureAssets& textures, const graphics::VisibleArea& visible,
               graphics::SpriteQueue& queue) {
    const std::optional<contract::renderer::TextureId> texture = textures.getTexture(layer.image);
    const graphics::TextureAssets::Entry* entry = textures.get(layer.image);
    if (!texture || entry == nullptr) {
        return;
    }
    if (const auto sprite = detail::imageSprite(layer, *texture, entry->desc.size, placement, visible)) {
        queue.push(drawLayer, *sprite);
    }
}

/// @brief Render, client: pushes the visible layers to the SpriteQueue.
void drawTilemaps(ecs::Res<TilemapAssets> maps, ecs::Res<graphics::TextureAssets> textures,
                  ecs::Res<graphics::VisibleArea> visible, ecs::ResAppend<graphics::SpriteQueue> queue,
                  ecs::Query<const transform::GlobalTransform, const TilemapLayer> layers) {
    if (visible->isEmpty()) {
        return;
    }
    for (auto [entity, global, drawn] : layers) {
        const Tilemap* map = maps->get(drawn.map);
        if (map == nullptr || drawn.layer >= map->getLayers().size()) {
            continue;
        }
        const MapLayer& layer = map->getLayers().at(drawn.layer);
        if (!layer.visible) {
            continue;
        }
        const detail::LayerPlacement placement{.position = global.position, .scale = global.scale};
        if (layer.kind == LayerKind::kTiles) {
            drawTiles(*map, layer, drawn.drawLayer, placement, *textures, *visible, *queue);
        } else {
            drawImage(layer, drawn.drawLayer, placement, *textures, *visible, *queue);
        }
    }
}
}  // namespace

std::string_view TilemapPlugin::getName() const noexcept { return kName; }

std::span<const std::string_view> TilemapPlugin::getDependencies() const noexcept {
    static constexpr std::array<std::string_view, 1> kDependencies{graphics::GraphicsPlugin::kName};
    return kDependencies;
}

void TilemapPlugin::build(ecs::App& app) {
    app.addPlugin(transform::TransformPlugin{});
    ecs::Resources& resources = app.getWorld().getResources();
    resources.insertIfAbsent<TilemapAssets>();
    resources.insertIfAbsent<graphics::TextureAssets>();
    resources.insertIfAbsent<graphics::SpriteQueue>();
    resources.insertIfAbsent<graphics::VisibleArea>();
    static_cast<void>(app.getWorld().getComponents().registerType<TilemapLayer>());
    static_cast<void>(app.getWorld().getComponents().registerType<Parallax>());
    app.addSystem(ecs::Phase::kRender, std::string{kParallaxSystem}, &applyParallax)
        .before(std::string{graphics::GraphicsPlugin::kCameraSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kTexturesSystem}, &queueMapTextures)
        .before(std::string{graphics::GraphicsPlugin::kTexturesSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kDrawSystem}, &drawTilemaps)
        .after(std::string{graphics::GraphicsPlugin::kCameraSystem})
        .before(std::string{graphics::GraphicsPlugin::kSpritesSystem})
        .setSide(ecs::Side::kClient);
}

}  // namespace rtype::plugins::tilemap
