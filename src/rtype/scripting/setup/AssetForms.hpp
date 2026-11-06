/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetForms
*/

#pragma once

#include "rtype/ecs/app/App.hpp"
#include "rtype/luau/Library.hpp"
#include "rtype/luau/Runtime.hpp"

namespace rtype::scripting::setup {
/// @brief Builds `engine.assets`, the forms that ask the asset plugins for assets while scripts load.
///
/// @details Each form returns the asset's `AssetId` and makes the engine calls of its plugin:
/// - `texture(path, {linear, wrap})`: `rtype.backend.loadTexture`;
/// - `sound(path, {stream})`: `rtype.audio.loadSound`;
/// - `font(path)`: `rtype.text.loadFont`;
/// - `tilemap(path)`: `rtype.tilemap.load`;
/// - `atlas{...}`: an atlas file (`path`, `rtype.sprite.loadAtlas`) or an atlas made from a texture (`texture`,
///   `rtype.sprite.createAtlas`, with an optional `name`, `grid` and `frames`, `rtype.sprite.addFrame`), then its
///   `animations` by name (`rtype.sprite.addAnimation`), whose `frames` list consecutive frame indices.
///
/// A form checks its whole table first, then skips its calls when the App does not have them (the plugin is not in
/// this process, such as the sprite plugin on a dedicated server) and returns the id the asset would have, the hash
/// of its path or name. An unknown key, a value of the wrong type or a failing call is a Luau error naming the form.
/// @param runtime The scripts' runtime; it must outlive the library.
/// @param app The App the scripts set up; it must outlive the forms' calls.
/// @return The `assets` library.
[[nodiscard]] luau::Library buildAssetForms(luau::Runtime& runtime, ecs::App& app);
}  // namespace rtype::scripting::setup
