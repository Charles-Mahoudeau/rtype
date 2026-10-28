/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Font
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "core/asset/AssetId.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/math/Rect.hpp"

namespace rtype::plugins::text {

/// @brief The metrics of one character of a Font.
struct Glyph {
    float advance = 0.0F;  ///< How far the pen moves after it, in ems.
    /// Where it is drawn, in ems from the pen on the baseline, Y down: its top is negative. Empty for a blank (space).
    backend::math::Rect plane;
    backend::math::Rect atlas;  ///< Where it is in the atlas image, in pixels. Empty for a blank.

    /// @return Whether it draws nothing (a space): it only moves the pen.
    [[nodiscard]] bool isBlank() const noexcept { return plane.getWidth() <= 0.0F || plane.getHeight() <= 0.0F; }
};

/// @brief A font as a multi-channel signed distance field (MSDF) atlas: the image of its glyphs and their metrics, made
/// offline by msdf-atlas-gen. Every length is in ems (the font size), Y down, from the baseline.
///
/// @details Built by detail::parseFontJson() from msdf-atlas-gen's JSON, or in code. layoutText() places its glyphs.
class Font {
  public:
    /// @brief The vertical metrics of the font, in ems from the baseline, Y down.
    struct Metrics {
        float lineHeight = 1.0F;  ///< From one baseline to the next.
        float ascender = -1.0F;   ///< The top of the tallest glyphs: negative, above the baseline.
        float descender = 0.0F;   ///< The bottom of the lowest glyphs: positive, below the baseline.
    };

    /// @param texturePath The atlas image: TextureAssets loads it with @p textureDesc.
    /// @param textureDesc Its size, and how it is sampled: linear filtering and TextureEncoding::kLinear, for the
    /// distances to be read as they are.
    /// @param distanceRange The width of the distance field around the edges, in atlas pixels.
    Font(std::string texturePath, const backend::graphics::TextureDesc& textureDesc, float distanceRange,
         const Metrics& metrics);
    ~Font() = default;
    Font(const Font& other) = default;
    Font& operator=(const Font& other) = default;
    Font(Font&& other) noexcept = default;
    Font& operator=(Font&& other) noexcept = default;

    /// @return The path of the atlas image.
    [[nodiscard]] const std::string& getTexturePath() const noexcept { return _texturePath; }
    /// @return The id of the atlas image in TextureAssets.
    [[nodiscard]] core::asset::AssetId getTexture() const noexcept { return _texture; }
    /// @return How the atlas image is sampled, and its size.
    [[nodiscard]] const backend::graphics::TextureDesc& getTextureDesc() const noexcept { return _textureDesc; }
    /// @return The width of the distance field, in atlas pixels.
    [[nodiscard]] float getDistanceRange() const noexcept { return _distanceRange; }
    /// @return The vertical metrics.
    [[nodiscard]] const Metrics& getMetrics() const noexcept { return _metrics; }
    /// @return The number of glyphs.
    [[nodiscard]] std::size_t getGlyphCount() const noexcept { return _glyphs.size(); }

    /// @brief Adds or replaces the glyph of the character @p codepoint.
    void addGlyph(char32_t codepoint, const Glyph& glyph);

    /// @brief Sets how much closer (negative) or further the pen moves between @p left and @p right, in ems.
    void setKerning(char32_t left, char32_t right, float advance);

    /// @return The glyph of @p codepoint, or nullptr if the font does not have it.
    [[nodiscard]] const Glyph* findGlyph(char32_t codepoint) const;

    /// @return The adjustment between @p left and @p right, in ems; 0 for a pair without one.
    [[nodiscard]] float getKerning(char32_t left, char32_t right) const;

  private:
    /// @return The key of the pair @p left, @p right in the kerning table.
    [[nodiscard]] static std::uint64_t pairKey(char32_t left, char32_t right) noexcept;

    std::string _texturePath;                           ///< The atlas image.
    core::asset::AssetId _texture;                      ///< AssetId::fromPath(_texturePath).
    backend::graphics::TextureDesc _textureDesc;        ///< Its size and sampling.
    float _distanceRange;                               ///< In atlas pixels.
    Metrics _metrics;                                   ///< Vertical metrics.
    std::unordered_map<char32_t, Glyph> _glyphs;        ///< By codepoint.
    std::unordered_map<std::uint64_t, float> _kerning;  ///< By pairKey().
};

}  // namespace rtype::plugins::text
