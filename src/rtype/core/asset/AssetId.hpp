/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetId
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>

namespace rtype::core::asset {

/// @brief Names an asset (a texture...) the same way in every process: the hash of its path.
///
/// @details What components keep instead of a backend id: a TextureId is a slot of one renderer, meaningless to the
/// server, to Luau or in a save, while an AssetId is computed from the path alone, so every process finds the same one.
/// The ECS reflects it as FieldType::kAssetId (plugins/AssetIdReflection.hpp). The TextureAssets resource maps it to
/// the renderer's id once the texture is loaded.
struct AssetId {
    std::uint64_t value{0};  ///< FNV-1a hash of the normalized path; 0 means "no asset".

    /// @return The id of the asset at @p path, normalized first (`a/./b/../c.png` gives the id of `a/c.png`, and so
    /// does `a\c.png` on Windows). Never 0.
    [[nodiscard]] static AssetId fromPath(const std::filesystem::path& path);

    [[nodiscard]] bool operator==(const AssetId& other) const = default;
};

}  // namespace rtype::core::asset

/// @brief Hashes an AssetId for unordered containers: it already is a hash.
template <>
struct std::hash<rtype::core::asset::AssetId> {
    [[nodiscard]] std::size_t operator()(rtype::core::asset::AssetId id) const noexcept {
        return static_cast<std::size_t>(id.value);
    }
};
