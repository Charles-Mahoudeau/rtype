/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FontJson
*/

#pragma once

#include <filesystem>
#include <string_view>

#include "plugins/text/Font.hpp"

namespace rtype::plugins::text::detail {

/// @brief Builds a font from the JSON msdf-atlas-gen writes next to its atlas image (`-json`).
///
/// @details Reads `atlas` (`type`, which must be "msdf" or "mtsdf", `distanceRange`, `width`, `height` and
/// `yOrigin`), `metrics` (`lineHeight`, `ascender`, `descender`), each of `glyphs` (`unicode`, `advance`, and for a
/// visible glyph `planeBounds` and `atlasBounds`) and `kerning` (`unicode1`, `unicode2`, `advance`). Lengths are kept
/// Y down: a file written with `-yorigin bottom` is flipped. The image is not named in the file: it is @p path with the
/// extension `.png`, as `msdf-atlas-gen -imageout <name>.png -json <name>.json` writes them.
/// @param text The file's content.
/// @param path Where it was read from: names the font in errors and gives the image.
/// @throws exceptions::FontException If the text is not JSON, lacks or mistypes a field this reads, or is not an MSDF
/// atlas.
[[nodiscard]] Font parseFontJson(std::string_view text, const std::filesystem::path& path);

}  // namespace rtype::plugins::text::detail
