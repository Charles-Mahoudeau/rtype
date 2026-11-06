/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextureCalls
*/

#pragma once

#include <string_view>

#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/call/CallTraits.hpp"
#include "rtype/ecs/type/String.hpp"

namespace rtype::plugins::graphics {

inline constexpr std::string_view kLoadTextureCall = "rtype.backend.loadTexture";  ///< TextureAssets::load().

/// @brief The arguments of `rtype.backend.loadTexture`: asks for an image file.
struct LoadTextureArguments {
    ecs::String path;    ///< The image file.
    bool linear{false};  ///< Smooth sampling instead of sharp pixels.
    bool wrap{false};  ///< Tiles the texture when a source rectangle goes past its edges (`repeat` is a Luau keyword).
};
RTYPE_ECS_CALL_DATA(LoadTextureArguments, "rtype.backend.LoadTextureArguments", path, linear, wrap);

/// @brief The result of `rtype.backend.loadTexture`.
struct TextureResult {
    core::asset::AssetId texture;  ///< The texture's id, for a SpriteRenderer.
};
RTYPE_ECS_CALL_DATA(TextureResult, "rtype.backend.TextureResult", texture);

/// @brief Registers the texture engine call on @p app, for scripts: `rtype.backend.loadTexture`, on the
/// `rtype.TextureAssets` resource. Called by the GraphicsPlugin.
void addTextureCalls(ecs::App& app);

}  // namespace rtype::plugins::graphics
