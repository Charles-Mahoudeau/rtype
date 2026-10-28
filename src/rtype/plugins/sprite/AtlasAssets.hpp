/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AtlasAssets
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include "plugins/asset/AssetId.hpp"
#include "plugins/asset/AssetStore.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::sprite {

/// @brief The sprite atlases, as the ECS resource "rtype.AtlasAssets" the SpritePlugin inserts: systems ask for an
/// atlas and get its AssetId at once, SpriteAnimator components keep that id.
///
/// @details An atlas is read when it is asked for, on every side (a server gets the same frames and animations), so its
/// frames can be looked up at once. Its texture is not: the SpritePlugin hands each atlas' texture to TextureAssets on
/// the client, which loads it before the frame that draws it. An atlas that cannot be built is recorded as failed, with
/// the reason, which is also printed: one missing atlas does not stop the game.
/// @code
/// void spawnShip(rtype::ecs::Commands& commands, rtype::ecs::ResMut<rtype::plugins::sprite::AtlasAssets> atlases)
/// {
///     const rtype::plugins::asset::AssetId ship = atlases->load("assets/ship.json");
///     commands.spawn()
///         .add(rtype::plugins::transform::Transform{.position = {100.0F, 300.0F}})
///         .add(rtype::plugins::backend::SpriteRenderer{})
///         .add(rtype::plugins::sprite::SpriteAnimator{.atlas = ship,
///                                                     .animation = rtype::plugins::sprite::animationId("idle")});
/// }
/// @endcode
class AtlasAssets {
  public:
    /// @brief One atlas: its file (or the name it was added or packed under), the atlas (nothing if it could not be
    /// built) and why it could not be.
    using Entry = asset::AssetStore<SpriteAtlas>::Entry;

    /// @brief A texture an atlas needs, for TextureAssets.
    struct TextureRequest {
        std::string path;                     ///< The image file, or the name of the pixels.
        backend::graphics::TextureDesc desc;  ///< Filter and repeat mode; the size of the pixels.
        std::vector<std::byte> pixels;        ///< RGBA pixels of a packed atlas; empty for an image file.
    };

    AtlasAssets() = default;
    ~AtlasAssets() = default;
    AtlasAssets(const AtlasAssets& other) = delete;
    AtlasAssets& operator=(const AtlasAssets& other) = delete;
    AtlasAssets(AtlasAssets&& other) noexcept = default;
    AtlasAssets& operator=(AtlasAssets&& other) noexcept = default;

    /// @return The number of atlases known, built or failed.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @brief Reads the Aseprite or TexturePacker JSON file at @p path (detail::parseAtlasJson()): its frames, its
    /// animations (Aseprite's frame tags) and its image, relative to the file.
    /// @param desc How the image is sampled.
    /// @return Its id (AssetId::fromPath()), at once, even if it failed. Asking twice gives the first one.
    /// @throws backend::exceptions::AssetLoadException If another path already has this id (a hash collision).
    asset::AssetId load(const std::filesystem::path& path, const backend::graphics::TextureDesc& desc = {});

    /// @brief Adds an atlas built in code (SpriteAtlas::grid(), addFrame(), addAnimation()), known by @p name.
    /// @return Its id (AssetId::fromPath(name)). Adding a name twice keeps the first atlas.
    /// @throws backend::exceptions::AssetLoadException If another name already has this id (a hash collision).
    asset::AssetId add(const std::string& name, SpriteAtlas atlas);

    /// @brief Packs the image files @p images into one texture (detail::packImages()), one frame per image named after
    /// its file's stem, its origin at its centre. Add animations to it with add() of a copy, or look its frames up.
    /// @param padding Transparent pixels between two images.
    /// @param desc How the packed texture is sampled; its size is ignored.
    /// @return Its id (AssetId::fromPath(name)), at once, even if it failed. Packing a name twice keeps the first.
    /// @throws backend::exceptions::AssetLoadException If another name already has this id (a hash collision).
    asset::AssetId pack(const std::string& name, std::span<const std::filesystem::path> images,
                        std::uint32_t padding = 1, const backend::graphics::TextureDesc& desc = {});

    /// @return The atlas @p id names, or nullptr if it is unknown or failed.
    [[nodiscard]] const SpriteAtlas* get(asset::AssetId id) const;

    /// @return The entry of @p id, built or failed, or nullptr if it is unknown.
    [[nodiscard]] const Entry* getEntry(asset::AssetId id) const;

    /// @brief Forgets the atlas @p id names. Its texture stays in TextureAssets: other atlases may share it.
    /// @return False, doing nothing, if no atlas has this id.
    bool unload(asset::AssetId id);

    /// @return The textures the atlases added since the last call need, for TextureAssets. Called by the SpritePlugin.
    [[nodiscard]] std::vector<TextureRequest> takeTextureRequests();

  private:
    /// @brief Asks for the texture of @p atlas, just stored: @p pixels if given, its image file otherwise.
    void requestTexture(const SpriteAtlas& atlas, std::vector<std::byte> pixels = {});

    asset::AssetStore<SpriteAtlas> _atlases{"Atlas"};  ///< Every atlas asked for, by id.
    std::vector<TextureRequest> _textureRequests;      ///< Textures to hand to TextureAssets.
};
RTYPE_ECS_RESOURCE(AtlasAssets, "rtype.AtlasAssets");

}  // namespace rtype::plugins::sprite
