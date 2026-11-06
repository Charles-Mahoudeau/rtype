/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TilemapCalls
*/

#include "plugins/tilemap/TilemapCalls.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

#include "plugins/tilemap/SpawnTilemap.hpp"
#include "plugins/tilemap/TilemapAssets.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

namespace rtype::plugins::tilemap {
namespace {
TilemapResult loadTilemap(ecs::ResMut<TilemapAssets> maps, const LoadTilemapArguments& arguments) {
    return TilemapResult{.map = maps->load(std::filesystem::path{arguments.path.view()})};
}

SpawnTilemapResult spawn(ecs::Res<TilemapAssets> maps, ecs::Commands& commands,
                         const SpawnTilemapArguments& arguments) {
    const TilemapPlacement placement{
        .position = arguments.position,
        .scale = arguments.scale,
        .firstDrawLayer = arguments.firstDrawLayer,
    };
    const auto layers = spawnTilemap(commands, *maps, arguments.map, placement).size();
    return SpawnTilemapResult{.layers = static_cast<std::uint32_t>(layers)};
}
}  // namespace

void addTilemapCalls(ecs::App& app) {
    app.addCall(std::string{kLoadTilemapCall}, &loadTilemap);
    app.addCall(std::string{kSpawnTilemapCall}, &spawn);
}

}  // namespace rtype::plugins::tilemap
