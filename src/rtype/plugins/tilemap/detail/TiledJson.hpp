/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TiledJson
*/

#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>

#include "plugins/tilemap/Tilemap.hpp"

namespace rtype::plugins::tilemap::detail {

/// @brief Reads the text of a file; TilemapAssets reads it from disk, tests from memory.
using TextReader = std::function<std::string(const std::filesystem::path& path)>;

/// @brief Builds a map from Tiled's JSON format (a `.tmj` file).
///
/// @details Reads an orthogonal map, finite or infinite (its tile layers' `chunks`), and:
/// - its tilesets, embedded or in an external `.tsj` file (`source`, read with @p readText): each one image cut into
///   tiles (`image`, `tilewidth`, `tileheight`, `columns`, `tilecount`, `spacing`, `margin`, `tileoffset`);
/// - its tile layers (`data` as a JSON array, Tiled's "CSV" encoding) and image layers (`image`, `repeatx`,
///   `repeaty`), each with `offsetx`, `offsety`, `parallaxx`, `parallaxy`, `opacity`, `tintcolor` and `visible`;
/// - its group layers, flattened into the layers they hold;
/// - `parallaxoriginx` and `parallaxoriginy`, and the custom float properties `scrollx` and `scrolly` of a layer
///   (pixels per second it scrolls on its own).
/// Object layers are skipped. Image paths are relative to the file that names them.
/// @param text The map file's content.
/// @param path Where it was read from: names the map in errors and resolves relative paths.
/// @param readText Reads an external tileset (asset::readTextFile() for files on disk).
/// @throws exceptions::TilemapException Naming the map, if a file is not JSON, lacks or mistypes a field this reads,
/// uses what this does not support (another orientation, base64 tile data, a tileset of separate images), or cannot be
/// read by @p readText.
[[nodiscard]] Tilemap parseTiledMap(std::string_view text, const std::filesystem::path& path,
                                    const TextReader& readText);

}  // namespace rtype::plugins::tilemap::detail
