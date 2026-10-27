/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TilemapPlugin
*/

#include "plugins/tilemap/TilemapPlugin.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/graphics/Camera.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/SpriteQueue.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/backend/graphics/VisibleArea.hpp"
#include "plugins/tilemap/Parallax.hpp"
#include "plugins/tilemap/Tilemap.hpp"
#include "plugins/tilemap/TilemapAssets.hpp"
#include "plugins/tilemap/TilemapLayer.hpp"
#include "plugins/tilemap/detail/TileDrawing.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/change/Mut.hpp"
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
void applyParallax(
    ecs::Res<ecs::Time> time,
    ecs::Query<const transform::GlobalTransform, const backend::graphics::Camera, ecs::Without<Parallax>> cameras,
    ecs::Query<const transform::Transform, Parallax, transform::GlobalTransform> moved) {
    std::optional<glm::vec2> camera;
    std::int32_t priority = 0;
    for (auto [entity, global, shown] : cameras) {
        if (!camera || shown.priority > priority) {
            camera = global.position;
            priority = shown.priority;
        }
    }
    for (auto [entity, local, parallax, global] : moved) {
        if (parallax.get().scroll != glm::vec2{0.0F}) {
            parallax->scrolled += parallax.get().scroll * time->delta;
        }
        const Parallax& current = parallax.get();
        const glm::vec2 follow =
            camera ? (*camera - current.origin) * (glm::vec2{1.0F} - current.factor) : glm::vec2{0.0F};
        const glm::vec2 position = local.position + follow + current.scrolled;
        if (global.get().position != position || global.get().scale != local.scale ||
            global.get().rotation != local.rotation) {
            global->position = position;
            global->scale = local.scale;
            global->rotation = local.rotation;
        }
    }
}

/// @brief Render, client: hands the maps' images to TextureAssets.
void queueMapTextures(ecs::ResMut<TilemapAssets> maps, ecs::ResMut<backend::TextureAssets> textures) {
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
               const detail::LayerPlacement& placement, const backend::TextureAssets& textures,
               const backend::graphics::VisibleArea& visible, backend::graphics::SpriteQueue& queue) {
    const std::vector<Tileset>& tilesets = map.getTilesets();
    std::vector<std::optional<backend::graphics::TextureId>> loaded;
    loaded.reserve(tilesets.size());
    for (const Tileset& tileset : tilesets) {
        loaded.push_back(textures.getTexture(tileset.texture));
    }
    const backend::graphics::Color color = detail::layerColor(layer);
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
            const std::optional<backend::graphics::TextureId>& texture =
                loaded.at(static_cast<std::size_t>(std::distance(tilesets.data(), tileset)));
            if (texture) {
                queue.push(drawLayer, detail::tileSprite(map, *tileset, *texture, gid, cell, placement, color));
            }
        }
    }
}

/// @brief Pushes the image of the image layer @p layer, if it is loaded and visible.
void drawImage(const MapLayer& layer, std::int32_t drawLayer, const detail::LayerPlacement& placement,
               const backend::TextureAssets& textures, const backend::graphics::VisibleArea& visible,
               backend::graphics::SpriteQueue& queue) {
    const std::optional<backend::graphics::TextureId> texture = textures.getTexture(layer.image);
    const backend::TextureAssets::Entry* entry = textures.get(layer.image);
    if (!texture || entry == nullptr) {
        return;
    }
    if (const auto sprite = detail::imageSprite(layer, *texture, entry->desc.size, placement, visible)) {
        queue.push(drawLayer, *sprite);
    }
}

/// @brief Render, client: pushes the visible layers to the SpriteQueue.
void drawTilemaps(ecs::Res<TilemapAssets> maps, ecs::Res<backend::TextureAssets> textures,
                  ecs::Res<backend::graphics::VisibleArea> visible, ecs::ResMut<backend::graphics::SpriteQueue> queue,
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

void TilemapPlugin::build(ecs::App& app) {
    app.addPlugin(transform::TransformPlugin{});
    ecs::Resources& resources = app.getWorld().getResources();
    if (!resources.has<TilemapAssets>()) {
        resources.insert(TilemapAssets{});
    }
    if (!resources.has<backend::TextureAssets>()) {
        resources.insert(backend::TextureAssets{});
    }
    if (!resources.has<backend::graphics::SpriteQueue>()) {
        resources.insert(backend::graphics::SpriteQueue{});
    }
    if (!resources.has<backend::graphics::VisibleArea>()) {
        resources.insert(backend::graphics::VisibleArea{});
    }
    static_cast<void>(app.getWorld().getComponents().registerType<TilemapLayer>());
    static_cast<void>(app.getWorld().getComponents().registerType<Parallax>());
    app.addSystem(ecs::Phase::kRender, std::string{kParallaxSystem}, &applyParallax)
        .before(std::string{backend::BackendPlugin::kCameraSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kTexturesSystem}, &queueMapTextures)
        .before(std::string{backend::BackendPlugin::kTexturesSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kDrawSystem}, &drawTilemaps)
        .after(std::string{backend::BackendPlugin::kCameraSystem})
        .before(std::string{backend::BackendPlugin::kSpritesSystem})
        .setSide(ecs::Side::kClient);
}

}  // namespace rtype::plugins::tilemap
