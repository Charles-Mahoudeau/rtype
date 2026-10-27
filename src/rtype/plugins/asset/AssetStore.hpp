/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetStore
*/

#pragma once

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "plugins/asset/AssetId.hpp"

namespace rtype::plugins::asset {

/// @brief The assets of one kind read from files or built in code, by AssetId: the bookkeeping the asset resources
/// (AtlasAssets, TilemapAssets, FontAssets) share, so each keeps only how its files are read and what it asks of
/// TextureAssets.
///
/// @details An asset is known by its normalized path (normalizePath()), or the name it was added under, and its id is
/// AssetId::fromPath() of it. Asking for a known one again gives its id without reading anything. An asset that cannot
/// be read is recorded as failed, with the reason, which is also printed: one missing asset does not stop the game.
/// @tparam T The asset: a SpriteAtlas, a Tilemap, a Font...
template <typename T>
class AssetStore {
  public:
    /// @brief One asset, read or failed.
    struct Entry {
        std::string path;        ///< Its file, or the name it was added under.
        std::optional<T> value;  ///< The asset; nothing if it could not be read.
        std::string error;       ///< Why it could not be read.
    };

    /// @brief What load() and add() did.
    struct Stored {
        AssetId id;                ///< The asset's id, read now, before, or failed.
        const T* added = nullptr;  ///< The asset if this call stored it; nullptr if it was known, or failed.
    };

    /// @param kind Names the assets in messages: "Atlas" gives "Atlas 'a.json': ..." and "[AtlasAssets] ...".
    explicit AssetStore(std::string kind);
    ~AssetStore() = default;
    AssetStore(const AssetStore& other) = delete;
    AssetStore& operator=(const AssetStore& other) = delete;
    AssetStore(AssetStore&& other) noexcept = default;
    AssetStore& operator=(AssetStore&& other) noexcept = default;

    /// @return The number of assets known, read or failed.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @return The asset @p id names, or nullptr if it is unknown or failed.
    [[nodiscard]] const T* get(AssetId id) const;

    /// @return The entry of @p id, read or failed, or nullptr if it is unknown.
    [[nodiscard]] const Entry* getEntry(AssetId id) const;

    /// @brief Reads the file at @p path with @p parse, unless it is already known.
    /// @param parse Builds the asset from the file's text and its path; it throws (any std::exception) when it cannot,
    /// with a message that names the file.
    /// @return Its id, at once, even if it failed, and the asset if it was read now.
    /// @throws backend::exceptions::AssetLoadException If another path already has this id (a hash collision).
    template <typename Parse>
        requires std::invocable<const Parse&, std::string_view, const std::filesystem::path&>
    Stored load(const std::filesystem::path& path, const Parse& parse);

    /// @brief Adds @p value, known by @p name, unless the name is already known (the first asset is kept).
    /// @return Its id, and the asset if it was added now.
    /// @throws backend::exceptions::AssetLoadException If another name already has this id (a hash collision).
    Stored add(const std::string& name, T value);

    /// @brief Forgets the asset @p id names.
    /// @return False, doing nothing, if no asset has this id.
    bool unload(AssetId id);

    /// @return The id of @p path, already normalized, or nothing if it is already known: for an asset built another
    /// way, then store() or fail().
    /// @throws backend::exceptions::AssetLoadException If another path already has the id.
    [[nodiscard]] std::optional<AssetId> reserve(const std::string& path) const;

    /// @brief Records @p value under @p path, reserved by reserve().
    /// @return The stored asset.
    const T& store(AssetId id, std::string path, T value);

    /// @brief Records that the asset @p path, reserved by reserve(), could not be read, and prints @p error.
    void fail(AssetId id, std::string path, std::string error);

  private:
    std::string _kind;                            ///< "Atlas", "Tilemap", "Font"...
    std::unordered_map<AssetId, Entry> _entries;  ///< Every asset asked for, by id.
};

}  // namespace rtype::plugins::asset

#include "AssetStore.tpp"
