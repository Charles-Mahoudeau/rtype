/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AtlasJson
*/

#pragma once

#include <filesystem>
#include <string_view>

#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"

namespace rtype::plugins::sprite::detail {

/// @brief Builds an atlas from the JSON Aseprite and TexturePacker export (the "JSON Hash" and "JSON Array" layouts).
///
/// @details Reads, for each frame in file order: `frame` (where it is in the image), `duration` (milliseconds, 100 when
/// absent), and for a trimmed frame `spriteSourceSize` and `sourceSize`, so the origin stays where it was in the
/// untrimmed image. The origin is TexturePacker's `pivot` when given (0 to 1 in the untrimmed frame), its centre
/// otherwise. `meta.image` is the texture, relative to the JSON file. Each of Aseprite's `meta.frameTags` becomes an
/// animation: its frames `from` to `to`, their durations, its `direction` (forward, reverse, pingpong,
/// pingpong_reverse) and its `repeat` count (forever when absent).
/// @param text The file's content.
/// @param path Where it was read from: names the atlas in errors and resolves `meta.image`.
/// @param desc How the image is sampled.
/// @throws exceptions::AtlasException If the text is not JSON, or lacks or mistypes a field this reads.
[[nodiscard]] SpriteAtlas parseAtlasJson(std::string_view text, const std::filesystem::path& path,
                                         const backend::graphics::TextureDesc& desc = {});

}  // namespace rtype::plugins::sprite::detail
