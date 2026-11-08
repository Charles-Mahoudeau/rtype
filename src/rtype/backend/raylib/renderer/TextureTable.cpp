/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextureTable
*/

#include "TextureTable.hpp"

#include <raylib.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>

#include "rtype/contract/renderer/drawing/Texture.hpp"

namespace rtype::backend::raylib::renderer {

TextureTable::~TextureTable() {
    if (!IsWindowReady()) {
        return;
    }
    for (const std::optional<::Texture2D>& texture : _textures) {
        if (texture) {
            UnloadTexture(*texture);
        }
    }
}

contract::renderer::TextureId TextureTable::create(const contract::renderer::TextureDesc& desc,
                                                   std::span<const std::byte> pixels) {
    constexpr std::size_t kBytesPerPixel = 4;
    const std::size_t expected = static_cast<std::size_t>(desc.size.x) * desc.size.y * kBytesPerPixel;
    if (desc.size.x == 0 || desc.size.y == 0 || pixels.size() != expected) {
        throw std::invalid_argument("createTexture: the pixels do not match the size (RGBA, 8 bits per channel)");
    }
    // raylib only reads the image: LoadTextureFromImage() copies it to the GPU.
    const ::Image image{.data = const_cast<std::byte*>(pixels.data()),  // NOLINT(cppcoreguidelines-pro-type-const-cast)
                        .width = static_cast<int>(desc.size.x),
                        .height = static_cast<int>(desc.size.y),
                        .mipmaps = 1,
                        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    const ::Texture2D texture = LoadTextureFromImage(image);
    const bool linear = desc.filter == contract::renderer::TextureFilter::kLinear;
    SetTextureFilter(texture, linear ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
    SetTextureWrap(texture, desc.repeat ? TEXTURE_WRAP_REPEAT : TEXTURE_WRAP_CLAMP);

    std::uint32_t index = 0;
    if (!_freeSlots.empty()) {
        index = _freeSlots.back();
        _freeSlots.pop_back();
        _textures.at(index) = texture;
    } else {
        index = static_cast<std::uint32_t>(_textures.size());
        _textures.emplace_back(texture);
        _generations.push_back(0);
    }
    return {.index = index, .generation = ++_generations.at(index)};
}

void TextureTable::destroy(contract::renderer::TextureId id) {
    const ::Texture2D* texture = find(id);
    if (texture == nullptr) {
        return;
    }
    UnloadTexture(*texture);
    _textures.at(id.index).reset();
    ++_generations.at(id.index);
    _freeSlots.push_back(id.index);
}

const ::Texture2D* TextureTable::find(contract::renderer::TextureId id) const {
    if (id.generation == 0 || id.index >= _textures.size() || _generations.at(id.index) != id.generation) {
        return nullptr;
    }
    const std::optional<::Texture2D>& texture = _textures.at(id.index);
    return texture.has_value() ? &texture.value() : nullptr;
}

}  // namespace rtype::backend::raylib::renderer
