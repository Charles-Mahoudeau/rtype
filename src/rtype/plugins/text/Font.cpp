/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Font
*/

#include "plugins/text/Font.hpp"

#include <cstdint>
#include <string>
#include <utility>

#include "plugins/asset/AssetId.hpp"
#include "plugins/backend/graphics/Texture.hpp"

namespace rtype::plugins::text {

namespace {
/// @brief Bits of the left character in a kerning key: every Unicode codepoint fits in 21.
constexpr std::uint32_t kCodepointBits = 32;
}  // namespace

Font::Font(std::string texturePath, const backend::graphics::TextureDesc& textureDesc, float distanceRange,
           const Metrics& metrics)
    : _texturePath{std::move(texturePath)},
      _texture{asset::AssetId::fromPath(_texturePath)},
      _textureDesc{textureDesc},
      _distanceRange{distanceRange},
      _metrics{metrics} {}

void Font::addGlyph(char32_t codepoint, const Glyph& glyph) { _glyphs.insert_or_assign(codepoint, glyph); }

void Font::setKerning(char32_t left, char32_t right, float advance) {
    _kerning.insert_or_assign(pairKey(left, right), advance);
}

const Glyph* Font::findGlyph(char32_t codepoint) const {
    const auto found = _glyphs.find(codepoint);
    return found != _glyphs.end() ? &found->second : nullptr;
}

float Font::getKerning(char32_t left, char32_t right) const {
    const auto found = _kerning.find(pairKey(left, right));
    return found != _kerning.end() ? found->second : 0.0F;
}

std::uint64_t Font::pairKey(char32_t left, char32_t right) noexcept {
    return (static_cast<std::uint64_t>(left) << kCodepointBits) | static_cast<std::uint64_t>(right);
}

}  // namespace rtype::plugins::text
