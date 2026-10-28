/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TilemapAssets
*/

#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "plugins/asset/AssetId.hpp"
#include "plugins/asset/AssetStore.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/tilemap/Tilemap.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::tilemap {

/// @brief The tilemaps, as the ECS resource "rtype.TilemapAssets" the TilemapPlugin inserts: systems ask for a map and
/// get its AssetId at once, then spawn its layers with spawnTilemap().
///
/// @details A map is read when it is asked for, on every side (a server gets the same tiles, for gameplay). Its images
/// are not: the TilemapPlugin hands them to TextureAssets on the client, which loads them before the frame that draws
/// them; an image layer that repeats asks for its texture with TextureDesc::repeat. A map that cannot be built is
/// recorded as failed, with the reason, which is also printed: one missing map does not stop the game.
/// @code
/// void spawnLevel(rtype::ecs::Commands& commands, rtype::ecs::ResMut<rtype::plugins::tilemap::TilemapAssets> maps) {
///     const rtype::plugins::asset::AssetId level = maps->load("assets/levels/space.tmj");
///     rtype::plugins::tilemap::spawnTilemap(commands, *maps, level);
/// }
/// @endcode
class TilemapAssets {
  public:
    /// @brief One map: its file (or the name it was added under), the map (nothing if it could not be built) and why it
    /// could not be.
    using Entry = asset::AssetStore<Tilemap>::Entry;

    /// @brief An image a map needs, for TextureAssets.
    struct TextureRequest {
        std::string path;                     ///< The image file.
        backend::graphics::TextureDesc desc;  ///< Repeat mode: on for an image layer that repeats.
    };

    TilemapAssets() = default;
    ~TilemapAssets() = default;
    TilemapAssets(const TilemapAssets& other) = delete;
    TilemapAssets& operator=(const TilemapAssets& other) = delete;
    TilemapAssets(TilemapAssets&& other) noexcept = default;
    TilemapAssets& operator=(TilemapAssets&& other) noexcept = default;

    /// @return The number of maps known, built or failed.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @brief Reads the Tiled map at @p path (detail::parseTiledMap()) and its external tilesets.
    /// @return Its id (AssetId::fromPath()), at once, even if it failed. Asking twice gives the first one.
    /// @throws backend::exceptions::AssetLoadException If another path already has this id (a hash collision).
    asset::AssetId load(const std::filesystem::path& path);

    /// @brief Adds a map built in code, known by @p name.
    /// @return Its id (AssetId::fromPath(name)). Adding a name twice keeps the first map.
    /// @throws backend::exceptions::AssetLoadException If another name already has this id (a hash collision).
    asset::AssetId add(const std::string& name, Tilemap map);

    /// @return The map @p id names, or nullptr if it is unknown or failed.
    [[nodiscard]] const Tilemap* get(asset::AssetId id) const;

    /// @return The entry of @p id, built or failed, or nullptr if it is unknown.
    [[nodiscard]] const Entry* getEntry(asset::AssetId id) const;

    /// @brief Forgets the map @p id names. Its textures stay in TextureAssets: other maps may share them.
    /// @return False, doing nothing, if no map has this id.
    bool unload(asset::AssetId id);

    /// @return The images the maps added since the last call need, for TextureAssets. Called by the TilemapPlugin.
    [[nodiscard]] std::vector<TextureRequest> takeTextureRequests();

  private:
    /// @brief Asks for the images of @p map, just stored, once each: repeated if an image layer repeats it.
    void requestTextures(const Tilemap& map);

    asset::AssetStore<Tilemap> _maps{"Tilemap"};   ///< Every map asked for, by id.
    std::vector<TextureRequest> _textureRequests;  ///< Images to hand to TextureAssets.
};
RTYPE_ECS_RESOURCE(TilemapAssets, "rtype.TilemapAssets");

}  // namespace rtype::plugins::tilemap
