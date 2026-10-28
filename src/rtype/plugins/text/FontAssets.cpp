/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FontAssets
*/

#include "plugins/text/FontAssets.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "plugins/asset/AssetId.hpp"
#include "plugins/asset/AssetStore.hpp"
#include "plugins/text/Font.hpp"
#include "plugins/text/detail/FontJson.hpp"

namespace rtype::plugins::text {

std::size_t FontAssets::getCount() const noexcept { return _fonts.getCount(); }

asset::AssetId FontAssets::load(const std::filesystem::path& path) {
    const auto stored = _fonts.load(path, &detail::parseFontJson);
    if (stored.added != nullptr) {
        requestTexture(*stored.added);
    }
    return stored.id;
}

asset::AssetId FontAssets::add(const std::string& name, Font font) {
    const auto stored = _fonts.add(name, std::move(font));
    if (stored.added != nullptr) {
        requestTexture(*stored.added);
    }
    return stored.id;
}

const Font* FontAssets::get(asset::AssetId id) const { return _fonts.get(id); }

const FontAssets::Entry* FontAssets::getEntry(asset::AssetId id) const { return _fonts.getEntry(id); }

bool FontAssets::unload(asset::AssetId id) { return _fonts.unload(id); }

std::vector<FontAssets::TextureRequest> FontAssets::takeTextureRequests() {
    return std::exchange(_textureRequests, {});
}

void FontAssets::requestTexture(const Font& font) {
    _textureRequests.push_back(TextureRequest{.path = font.getTexturePath(), .desc = font.getTextureDesc()});
}

}  // namespace rtype::plugins::text
