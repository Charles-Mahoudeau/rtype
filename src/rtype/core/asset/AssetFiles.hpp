/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetFiles
*/

#pragma once

#include <filesystem>
#include <string>

namespace rtype::core::asset {

/// @return @p path as assets are known by: lexically normalized, with forward slashes, as AssetId::fromPath() sees it
/// ("assets/./fonts/../fonts/a.json" and "assets\fonts\a.json" are both "assets/fonts/a.json").
[[nodiscard]] std::string normalizePath(const std::filesystem::path& path);

/// @brief Tells whether @p path is @p directory or something under it, once links are followed.
///
/// @details Both are normalized lexically (so "dir/a/../../x" is outside "dir"), then made canonical as far as they
/// exist (`weakly_canonical`), so a link inside @p directory that points out of it is outside. Never throws: a path
/// that cannot be resolved is reported as not inside.
/// @param directory The directory that must contain @p path.
/// @param path A path to test: absolute, or relative to the current directory like @p directory.
/// @return True if @p path is @p directory itself or lies under it.
[[nodiscard]] bool isInsideDirectory(const std::filesystem::path& directory,
                                     const std::filesystem::path& path) noexcept;

/// @brief Finds the file an asset path names in @p directory, refusing whatever leaves it: this is what lets a game
/// read its own assets and nothing else on the machine.
///
/// @details The check is made on the normalized path, so "levels/../backgrounds/a.png" is fine and "../a.png" is not.
/// A link inside the directory that points out of it is refused too.
/// @param directory The assets' directory; empty: no check, @p path is used as it is (a program without a game).
/// @param path The asset's path, relative to @p directory.
/// @return Where the file is.
/// @throws core::asset::exceptions::AssetLoadException If @p path is absolute or leaves @p directory.
[[nodiscard]] std::filesystem::path resolveAssetPath(const std::filesystem::path& directory,
                                                     const std::filesystem::path& path);

/// @return The whole content of the file at @p path, as bytes.
/// @throws core::asset::exceptions::AssetLoadException If it cannot be opened.
[[nodiscard]] std::string readTextFile(const std::filesystem::path& path);

}  // namespace rtype::core::asset
