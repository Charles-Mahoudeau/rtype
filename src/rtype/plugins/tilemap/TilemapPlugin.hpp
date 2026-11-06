/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TilemapPlugin
*/

#pragma once

#include <span>
#include <string_view>

#include "plugins/tilemap/Export.hpp"
#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::plugins::tilemap {

/// @brief Tile-based levels and parallax backgrounds: inserts the TilemapAssets resource, registers TilemapLayer and
/// Parallax, and draws the layers.
///
/// @details Three systems:
/// - Render "rtype.Tilemap.Parallax", on the client, before the GraphicsPlugin's camera: moves every entity with a
///   Parallax by the camera and its scroll (see Parallax). It is a visual effect, applied once gameplay and the
///   PostUpdate systems are done with GlobalTransform; the Render systems that draw run after the camera, so after it.
/// - Render "rtype.Tilemap.Textures", on the client, before the GraphicsPlugin loads the textures: hands the images of
///   the maps loaded since the last frame to TextureAssets, so a map loaded during Update is drawn in that frame.
/// - Render "rtype.Tilemap.Draw", on the client, after the GraphicsPlugin's camera and before its sprites: pushes to
/// the
///   SpriteQueue the tiles of each visible TilemapLayer whose chunk overlaps the VisibleArea, and its image for an
///   image layer, once their textures are loaded. They are sorted with the sprites by TilemapLayer::drawLayer.
/// It requires the GraphicsPlugin. Its Render system is ordered against the GraphicsPlugin's, so an App without a
/// window (a server) does not add it: it inserts TilemapAssets itself to read maps.
class RTYPE_PLUGIN_TILEMAP_API TilemapPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.Tilemap";                     ///< The plugin's name.
    static constexpr std::string_view kParallaxSystem = "rtype.Tilemap.Parallax";  ///< Moves the Parallax entities.
    static constexpr std::string_view kTexturesSystem = "rtype.Tilemap.Textures";  ///< Queues the map images.
    static constexpr std::string_view kDrawSystem = "rtype.Tilemap.Draw";          ///< Draws the layers.

    TilemapPlugin() = default;
    ~TilemapPlugin() override = default;
    TilemapPlugin(const TilemapPlugin& other) = default;
    TilemapPlugin& operator=(const TilemapPlugin& other) = default;
    TilemapPlugin(TilemapPlugin&& other) noexcept = default;
    TilemapPlugin& operator=(TilemapPlugin&& other) noexcept = default;

    /// @return "rtype.Tilemap".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @return The plugins this one needs.
    [[nodiscard]] std::span<const std::string_view> getDependencies() const noexcept override;

    /// @brief Inserts the resources, registers the components and adds the systems.
    void build(ecs::App& app) override;
};

}  // namespace rtype::plugins::tilemap
