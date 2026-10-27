/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextPlugin
*/

#pragma once

#include <string_view>

#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::plugins::text {

/// @brief Text drawn from MSDF font atlases: inserts the FontAssets resource, registers TextRenderer, and draws the
/// texts with the sprites.
///
/// @details Two systems:
/// - Render "rtype.Text.Textures", on the client, before the BackendPlugin loads the textures: hands the atlas images
///   of the fonts loaded since the last frame to TextureAssets, so a font loaded during Update is drawn in that frame.
///   It runs after the TilemapPlugin's "rtype.Tilemap.Textures", which also writes TextureAssets.
/// - Render "rtype.Text.Draw", on the client, after the BackendPlugin's camera and before its sprites: lays out each
///   TextRenderer whose font and atlas are loaded (layoutText()) and pushes it to the SpriteQueue as a graphics::Text,
///   at its entity's GlobalTransform, so it is sorted with the sprites by TextRenderer::layer. It runs after the
///   TilemapPlugin's "rtype.Tilemap.Draw", which pushes to the same queue (the schedule needs them ordered; the order
///   only breaks the ties of the sort).
/// The BackendPlugin adds it, after the TilemapPlugin. Its Render systems are ordered against the BackendPlugin's, so
/// an App without a window (a server) does not add it: it inserts FontAssets itself to measure texts.
class TextPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.Text";                     ///< The plugin's name.
    static constexpr std::string_view kTexturesSystem = "rtype.Text.Textures";  ///< Queues the atlas images.
    static constexpr std::string_view kDrawSystem = "rtype.Text.Draw";          ///< Draws the texts.

    TextPlugin() = default;
    ~TextPlugin() override = default;
    TextPlugin(const TextPlugin& other) = default;
    TextPlugin& operator=(const TextPlugin& other) = default;
    TextPlugin(TextPlugin&& other) noexcept = default;
    TextPlugin& operator=(TextPlugin&& other) noexcept = default;

    /// @return "rtype.Text".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Inserts the resources, registers the component and adds the systems.
    void build(ecs::App& app) override;
};

}  // namespace rtype::plugins::text
