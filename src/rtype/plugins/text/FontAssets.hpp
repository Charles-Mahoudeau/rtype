/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FontAssets
*/

#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "plugins/asset/AssetId.hpp"
#include "plugins/asset/AssetStore.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/text/Font.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::text {

/// @brief The fonts, as the ECS resource "rtype.FontAssets" the TextPlugin inserts: systems ask for a font and get its
/// AssetId at once, TextRenderer components keep that id.
///
/// @details A font is read when it is asked for, on every side (a server can measure texts too). Its atlas image is
/// not: the TextPlugin hands it to TextureAssets on the client, which loads it before the frame that draws it. A font
/// that cannot be read is recorded as failed, with the reason, which is also printed: one missing font does not stop
/// the game.
/// @code
/// void spawnTitle(rtype::ecs::Commands& commands, rtype::ecs::ResMut<rtype::plugins::text::FontAssets> fonts) {
///     commands.spawn()
///         .add(rtype::plugins::transform::Transform{.position = {400.0F, 100.0F}})
///         .add(rtype::plugins::text::TextRenderer{.text = "R-Type", .font = fonts->load("assets/fonts/orbitron.json"),
///                                                 .size = 64.0F, .anchor = {0.5F, 0.5F}});
/// }
/// @endcode
class FontAssets {
  public:
    /// @brief One font: its file (or the name it was added under), the font (nothing if it could not be read) and why
    /// it could not be.
    using Entry = asset::AssetStore<Font>::Entry;

    /// @brief An atlas image a font needs, for TextureAssets.
    struct TextureRequest {
        std::string path;                     ///< The image file.
        backend::graphics::TextureDesc desc;  ///< Linear filtering and encoding.
    };

    FontAssets() = default;
    ~FontAssets() = default;
    FontAssets(const FontAssets& other) = delete;
    FontAssets& operator=(const FontAssets& other) = delete;
    FontAssets(FontAssets&& other) noexcept = default;
    FontAssets& operator=(FontAssets&& other) noexcept = default;

    /// @return The number of fonts known, read or failed.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @brief Reads the msdf-atlas-gen JSON file at @p path (detail::parseFontJson()); its image is the same path with
    /// the extension `.png`.
    /// @return Its id (AssetId::fromPath()), at once, even if it failed. Asking twice gives the first one.
    /// @throws backend::exceptions::AssetLoadException If another path already has this id (a hash collision).
    asset::AssetId load(const std::filesystem::path& path);

    /// @brief Adds a font built in code, known by @p name.
    /// @return Its id (AssetId::fromPath(name)). Adding a name twice keeps the first font.
    /// @throws backend::exceptions::AssetLoadException If another name already has this id (a hash collision).
    asset::AssetId add(const std::string& name, Font font);

    /// @return The font @p id names, or nullptr if it is unknown or failed.
    [[nodiscard]] const Font* get(asset::AssetId id) const;

    /// @return The entry of @p id, read or failed, or nullptr if it is unknown.
    [[nodiscard]] const Entry* getEntry(asset::AssetId id) const;

    /// @brief Forgets the font @p id names. Its image stays in TextureAssets.
    /// @return False, doing nothing, if no font has this id.
    bool unload(asset::AssetId id);

    /// @return The images the fonts added since the last call need, for TextureAssets. Called by the TextPlugin.
    [[nodiscard]] std::vector<TextureRequest> takeTextureRequests();

  private:
    /// @brief Asks for the atlas image of @p font, just stored.
    void requestTexture(const Font& font);

    asset::AssetStore<Font> _fonts{"Font"};        ///< Every font asked for, by id.
    std::vector<TextureRequest> _textureRequests;  ///< Images to hand to TextureAssets.
};
RTYPE_ECS_RESOURCE(FontAssets, "rtype.FontAssets");

}  // namespace rtype::plugins::text
