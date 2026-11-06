/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TilemapCalls
*/

#include "plugins/tilemap/TilemapCalls.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

#include "plugins/tilemap/SpawnTilemap.hpp"
#include "plugins/tilemap/TilemapAssets.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
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

void addTilemapForms(ecs::PluginRegistry& registry) {
    static constexpr auto kForms = std::to_array<char>({
#include "TilemapForms.luau.h"
    });
    // bin2c ends the bytes with a null terminator, which is not part of the source.
    registry.addScriptForms("addTilemapForms", std::string_view{kForms.data(), kForms.size() - 1});
}

void addTilemapCalls(ecs::App& app) {
    addTilemapForms(app.getPluginRegistry());
    app.addCall(std::string{kLoadTilemapCall}, &loadTilemap);
    app.addCall(std::string{kSpawnTilemapCall}, &spawn);
}

}  // namespace rtype::plugins::tilemap
