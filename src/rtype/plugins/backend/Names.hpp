/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Names
*/

#pragma once

#include <string_view>

/// @brief The names plugins know each other by: each engine plugin's name, which another plugin lists in
/// getDependencies(), and the systems other plugins order themselves against with `after()` / `before()`.
/// @details Part of the contract, so a plugin names another one without including its header or linking its code. Each
/// plugin's own `kName` and `k…System` constants are these values.
/// @code
/// static constexpr std::array<std::string_view, 2> kDependencies{names::kTransform, names::kBackend};
/// app.addSystem(ecs::Phase::kRender, "game.hud", &drawHud).after(std::string{names::kBackendSprites});
/// @endcode
namespace rtype::plugins::backend::names {
// Plugins.
constexpr std::string_view kAudio = "rtype.audio";                ///< AudioPlugin.
constexpr std::string_view kBackend = "rtype.backend";            ///< BackendPlugin.
constexpr std::string_view kInput = "rtype.Input";                ///< InputPlugin.
constexpr std::string_view kParticle = "rtype.Particle";          ///< ParticlePlugin.
constexpr std::string_view kPhysics = "rtype.Physics";            ///< PhysicsPlugin.
constexpr std::string_view kPhysicsDebug = "rtype.PhysicsDebug";  ///< PhysicsDebugPlugin.
constexpr std::string_view kShader = "rtype.Shader";              ///< ShaderPlugin.
constexpr std::string_view kSprite = "rtype.Sprite";              ///< SpritePlugin.
constexpr std::string_view kText = "rtype.Text";                  ///< TextPlugin.
constexpr std::string_view kTilemap = "rtype.Tilemap";            ///< TilemapPlugin.
constexpr std::string_view kTransform = "rtype.Transform";        ///< TransformPlugin.

// Systems other plugins order themselves against.
constexpr std::string_view kBackendEvents = "rtype.backend.events";          ///< PreUpdate: sends the platform events.
constexpr std::string_view kBackendTextures = "rtype.backend.textures";      ///< Render: loads the textures.
constexpr std::string_view kBackendBeginFrame = "rtype.backend.beginFrame";  ///< Render: starts the frame.
constexpr std::string_view kBackendCamera = "rtype.backend.camera";          ///< Render: applies the camera.
constexpr std::string_view kBackendSprites = "rtype.backend.sprites";        ///< Render: draws the sprites.
constexpr std::string_view kBackendEndFrame = "rtype.backend.endFrame";      ///< Render: shows the frame.
constexpr std::string_view kTransformPropagate = "rtype.Transform.Propagate";  ///< PostUpdate: updates GlobalTransform.
constexpr std::string_view kTilemapTextures = "rtype.Tilemap.Textures";        ///< Render: queues the map images.
constexpr std::string_view kTilemapDraw = "rtype.Tilemap.Draw";                ///< Render: draws the layers.
constexpr std::string_view kTextDraw = "rtype.Text.Draw";                      ///< Render: draws the texts.
}  // namespace rtype::plugins::backend::names
