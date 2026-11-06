/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetDirectory
*/

#pragma once

#include <filesystem>

#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/resource/Resources.hpp"

namespace rtype::plugins {

/// @brief Where a game's assets are, as the ECS resource "rtype.AssetDirectory" the Engine inserts before the plugins
/// are built: `<game>/assets`. Every asset path is relative to it and cannot leave it
/// (core::asset::resolveAssetPath()). A program without an Engine has none: its paths are relative to the working
/// directory.
struct AssetDirectory {
    std::filesystem::path path;  ///< The directory.
};
RTYPE_ECS_RESOURCE(AssetDirectory, "rtype.AssetDirectory");

/// @return The directory @p resources hold, or an empty path if they hold none.
[[nodiscard]] inline std::filesystem::path assetDirectoryOf(const ecs::Resources& resources) {
    const auto* directory = resources.get<AssetDirectory>();
    return directory != nullptr ? directory->path : std::filesystem::path{};
}

}  // namespace rtype::plugins
