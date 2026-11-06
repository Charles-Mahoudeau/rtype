/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextCalls
*/

#pragma once

#include <string_view>

#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/call/CallTraits.hpp"
#include "rtype/ecs/type/String.hpp"

namespace rtype::plugins::text {

inline constexpr std::string_view kLoadFontCall = "rtype.text.loadFont";  ///< FontAssets::load().

/// @brief The arguments of `rtype.text.loadFont`: reads an msdf-atlas-gen JSON file.
struct LoadFontArguments {
    ecs::String path;  ///< The JSON file, next to its atlas image.
};
RTYPE_ECS_CALL_DATA(LoadFontArguments, "rtype.text.LoadFontArguments", path);

/// @brief The result of `rtype.text.loadFont`.
struct FontResult {
    core::asset::AssetId font;  ///< The font's id, for a TextRenderer.
};
RTYPE_ECS_CALL_DATA(FontResult, "rtype.text.FontResult", font);

/// @brief Registers the text engine call on @p app, for scripts: `rtype.text.loadFont`, on the `rtype.FontAssets`
/// resource. Called by the TextPlugin.
void addTextCalls(ecs::App& app);

}  // namespace rtype::plugins::text
