/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpritePlugin
*/

#pragma once

#include <string_view>

#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::plugins::sprite {

/// @brief Sprite atlases and animations: inserts the AtlasAssets resource (and TextureAssets if missing), registers
/// SpriteAnimator and AnimationFinished, and plays the animations.
///
/// @details Two PostUpdate systems, so gameplay switches animations in Update and the frame drawn is already the new
/// one:
/// - "rtype.Sprite.Animate", on every side: advances each playing SpriteAnimator by its speed, stops the finite ones on
///   their last frame (sending AnimationFinished), and writes the frame's texture, source and origin into the entity's
///   SpriteRenderer, if it has one, only when they change. Atlases and animations that do not exist (yet) are skipped.
/// - "rtype.Sprite.Textures", on the client, after it: hands the textures of the atlases added since the last frame to
///   TextureAssets, which the BackendPlugin loads before drawing.
/// The BackendPlugin adds it; a server without one adds it itself to keep its animators' time.
class SpritePlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.Sprite";                     ///< The plugin's name.
    static constexpr std::string_view kAnimateSystem = "rtype.Sprite.Animate";    ///< Plays the animations.
    static constexpr std::string_view kTexturesSystem = "rtype.Sprite.Textures";  ///< Queues the atlas textures.

    SpritePlugin() = default;
    ~SpritePlugin() override = default;
    SpritePlugin(const SpritePlugin& other) = default;
    SpritePlugin& operator=(const SpritePlugin& other) = default;
    SpritePlugin(SpritePlugin&& other) noexcept = default;
    SpritePlugin& operator=(SpritePlugin&& other) noexcept = default;

    /// @return "rtype.Sprite".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Inserts the resources, registers the component and the event, and adds the PostUpdate systems.
    void build(ecs::App& app) override;
};

}  // namespace rtype::plugins::sprite
