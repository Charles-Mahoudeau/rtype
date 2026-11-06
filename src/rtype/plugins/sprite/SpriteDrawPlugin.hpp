/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteDrawPlugin
*/

#pragma once

#include <span>
#include <string_view>

#include "plugins/sprite/Export.hpp"
#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::plugins::sprite {

/// @brief Draws the SpriteRenderer entities: the client side of the sprites, apart from the SpritePlugin so that a
/// server animates sprites without the GraphicsPlugin.
///
/// @details Render "rtype.Sprite.Draw", on the client, between the GraphicsPlugin's "rtype.graphics.camera" and
/// "rtype.graphics.sprites": pushes every entity with a GlobalTransform and a SpriteRenderer whose texture is loaded to
/// the SpriteQueue. A SpriteRenderer's material is used once the ShaderPlugin created it (ShaderHandles), the default
/// material until then, with the entity's MaterialInstance values if it has some. It requires the SpritePlugin, the
/// GraphicsPlugin, the ShaderPlugin and the TransformPlugin.
class RTYPE_PLUGIN_SPRITE_API SpriteDrawPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.SpriteDraw";         ///< The plugin's name.
    static constexpr std::string_view kDrawSystem = "rtype.Sprite.Draw";  ///< Pushes the sprites to the queue.

    SpriteDrawPlugin() = default;
    ~SpriteDrawPlugin() override = default;
    SpriteDrawPlugin(const SpriteDrawPlugin& other) = default;
    SpriteDrawPlugin& operator=(const SpriteDrawPlugin& other) = default;
    SpriteDrawPlugin(SpriteDrawPlugin&& other) noexcept = default;
    SpriteDrawPlugin& operator=(SpriteDrawPlugin&& other) noexcept = default;

    /// @return "rtype.SpriteDraw".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @return The SpritePlugin, the GraphicsPlugin, the ShaderPlugin and the TransformPlugin.
    [[nodiscard]] std::span<const std::string_view> getDependencies() const noexcept override;

    /// @brief Adds the draw system.
    void build(ecs::App& app) override;
};

}  // namespace rtype::plugins::sprite
