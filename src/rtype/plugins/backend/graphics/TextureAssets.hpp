/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextureAssets
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "plugins/asset/AssetId.hpp"
#include "plugins/backend/graphics/IRenderer.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::backend {
/// @brief Where a texture is in its loading.
enum class AssetState : std::uint8_t {
    kQueued,  ///< Asked for, waiting for the renderer to create it.
    kLoaded,  ///< Created by the renderer: its TextureId can be drawn.
    kFailed,  ///< Could not be loaded; the entry says why.
};

/// @brief The textures, as the ECS resource "rtype.TextureAssets" the BackendPlugin inserts: systems ask for a texture
/// by path and get its AssetId at once, components keep that id, and the BackendPlugin loads the queued textures.
///
/// @details load() records the texture as queued and returns its id, without touching the renderer: gameplay systems
/// need nothing but this resource, and a server without a renderer gets the same ids. Each frame, the BackendPlugin
/// calls updateTextures(), which creates the queued textures and destroys the unloaded ones. Lookups are by id, in
/// constant time.
/// @code
/// void spawnShip(rtype::ecs::Commands& commands, rtype::ecs::ResMut<rtype::plugins::backend::TextureAssets> textures)
/// {
///     const rtype::plugins::asset::AssetId ship = textures->load("assets/ship.png");
///     commands.spawn(rtype::plugins::backend::SpriteRenderer{.texture = ship});
/// }
/// @endcode
class TextureAssets {
  public:
    /// @brief One texture.
    struct Entry {
        std::string path;                       ///< Its path, or its name for an image given in memory.
        graphics::TextureDesc desc;             ///< Filter and repeat mode; the size is filled once loaded.
        AssetState state{AssetState::kQueued};  ///< Where it is in its loading.
        graphics::TextureId texture;            ///< The renderer's id, once loaded.
        std::vector<std::byte> encoded;         ///< The file's bytes, for an image given in memory; freed once loaded.
        std::vector<std::byte> pixels;          ///< RGBA pixels given by loadPixels(); freed once loaded.
        std::string error;                      ///< Why it failed to load.
    };

    TextureAssets() = default;
    ~TextureAssets() = default;
    TextureAssets(const TextureAssets& other) = delete;
    TextureAssets& operator=(const TextureAssets& other) = delete;
    TextureAssets(TextureAssets&& other) noexcept = default;
    TextureAssets& operator=(TextureAssets&& other) noexcept = default;

    /// @return The number of textures known, whatever their state.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @brief Asks for the image file at @p path (PNG, JPEG, BMP, TGA...), read from the disk by updateTextures().
    /// @param desc The filter and repeat mode; its size is ignored. Ignored if the texture was already asked for: the
    /// first request wins.
    /// @return Its id (AssetId::fromPath()), at once: put it in components, the texture is drawn once loaded.
    /// @throws exceptions::AssetLoadException If another path already has this id (a hash collision).
    asset::AssetId load(const std::filesystem::path& path, const graphics::TextureDesc& desc = {});

    /// @brief Asks for an image already in memory (embedded, downloaded), known by @p name.
    /// @param encoded The image file's bytes (PNG...), copied.
    /// @return Its id (AssetId::fromPath(name)).
    /// @throws exceptions::AssetLoadException If another name already has this id (a hash collision).
    asset::AssetId load(const std::string& name, std::span<const std::byte> encoded,
                        const graphics::TextureDesc& desc = {});

    /// @brief Asks for an image already decoded (built at runtime, e.g. a packed atlas), known by @p name.
    /// @param pixels RGBA pixels, 8 bits per channel, row by row from the top: desc.size.x * desc.size.y * 4 bytes,
    /// copied.
    /// @param desc Its size (required), filter and repeat mode.
    /// @return Its id (AssetId::fromPath(name)).
    /// @throws exceptions::AssetLoadException If the pixels do not match desc.size, or another name already has this
    /// id (a hash collision).
    asset::AssetId loadPixels(const std::string& name, std::span<const std::byte> pixels,
                              const graphics::TextureDesc& desc);

    /// @return The texture @p id names, or nullptr if it was never asked for or was unloaded.
    [[nodiscard]] const Entry* get(asset::AssetId id) const;

    /// @return The renderer's id of the texture @p id names, or nothing if it is not loaded (yet).
    [[nodiscard]] std::optional<graphics::TextureId> getTexture(asset::AssetId id) const;

    /// @brief Forgets the texture @p id names; if it was loaded, the next updateTextures() destroys it.
    /// @return False, doing nothing, if no texture has this id.
    bool unload(asset::AssetId id);

    /// @name For updateTextures()
    /// @{

    /// @return The ids of the queued textures, in the order they were asked for.
    [[nodiscard]] const std::vector<asset::AssetId>& getQueued() const noexcept;

    /// @brief Records that the queued texture @p id was created as @p texture with @p desc (its size filled), and
    /// frees its in-memory bytes. Does nothing if @p id is not queued.
    void setLoaded(asset::AssetId id, graphics::TextureId texture, const graphics::TextureDesc& desc);

    /// @brief Records that the queued texture @p id could not be loaded, and why. Does nothing if @p id is not queued.
    void setFailed(asset::AssetId id, std::string error);

    /// @return The textures unloaded since the last call, for the renderer to destroy.
    [[nodiscard]] std::vector<graphics::TextureId> takeUnloaded();
    /// @}

  private:
    /// @brief Records a queued texture, from its file, its encoded bytes or its pixels (at most one of them given), or
    /// returns the id of the one already asked for.
    /// @throws exceptions::AssetLoadException If another path already has the id.
    asset::AssetId request(std::string path, std::vector<std::byte> encoded, std::vector<std::byte> pixels,
                           const graphics::TextureDesc& desc);

    /// @return The queued entry @p id names, removed from the queue, or nullptr if it is not queued.
    Entry* dequeue(asset::AssetId id);

    std::unordered_map<asset::AssetId, Entry> _entries;  ///< Every texture asked for, by id.
    std::vector<asset::AssetId> _queued;                 ///< The queued textures, in the order they were asked for.
    std::vector<graphics::TextureId> _unloaded;          ///< Textures to destroy, from unload().
};
RTYPE_ECS_RESOURCE(TextureAssets, "rtype.TextureAssets");

/// @brief Destroys the textures unloaded since the last call, then loads the queued ones: decodes each image (through
/// stb_image, from its file or from the bytes given to load()) into RGBA pixels, or takes the pixels given to
/// loadPixels(), and creates the texture in @p renderer. Called by the BackendPlugin each frame, on the main thread.
/// @details An image that cannot be read, decoded or created is marked AssetState::kFailed with the reason, which is
/// also printed: one missing texture does not stop the game.
void updateTextures(TextureAssets& assets, graphics::IRenderer& renderer);
}  // namespace rtype::plugins::backend
