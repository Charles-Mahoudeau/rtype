/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpawnTilemap
*/

#include "plugins/tilemap/SpawnTilemap.hpp"

#include <cstdint>
#include <vector>

#include "plugins/asset/AssetId.hpp"
#include "plugins/tilemap/Parallax.hpp"
#include "plugins/tilemap/Tilemap.hpp"
#include "plugins/tilemap/TilemapAssets.hpp"
#include "plugins/tilemap/TilemapLayer.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::plugins::tilemap {

std::vector<ecs::Entity> spawnTilemap(ecs::Commands& commands, const TilemapAssets& maps, asset::AssetId map,
                                      const TilemapPlacement& placement) {
    std::vector<ecs::Entity> entities;
    const Tilemap* tilemap = maps.get(map);
    if (tilemap == nullptr) {
        return entities;
    }
    const auto origin = placement.position + (tilemap->getParallaxOrigin() * placement.scale);
    std::uint32_t index = 0;
    for (const MapLayer& layer : tilemap->getLayers()) {
        entities.push_back(
            commands.spawn()
                .add(transform::Transform{.position = placement.position + (layer.offset * placement.scale),
                                          .scale = placement.scale})
                .add(TilemapLayer{.map = map,
                                  .layer = index,
                                  .drawLayer = placement.firstDrawLayer + static_cast<std::int32_t>(index)})
                .add(Parallax{.factor = layer.parallax, .scroll = layer.scroll * placement.scale, .origin = origin})
                .getEntity());
        ++index;
    }
    return entities;
}

}  // namespace rtype::plugins::tilemap
