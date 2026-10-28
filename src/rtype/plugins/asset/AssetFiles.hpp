/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetFiles
*/

#pragma once

#include <filesystem>
#include <string>

namespace rtype::plugins::asset {

/// @return @p path as assets are known by: lexically normalized, with forward slashes, as AssetId::fromPath() sees it
/// ("assets/./fonts/../fonts/a.json" and "assets\fonts\a.json" are both "assets/fonts/a.json").
[[nodiscard]] std::string normalizePath(const std::filesystem::path& path);

/// @return The whole content of the file at @p path, as bytes.
/// @throws backend::exceptions::AssetLoadException If it cannot be opened.
[[nodiscard]] std::string readTextFile(const std::filesystem::path& path);

}  // namespace rtype::plugins::asset
