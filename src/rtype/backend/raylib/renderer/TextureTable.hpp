/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextureTable
*/

#pragma once

#include <raylib.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "rtype/contract/renderer/drawing/Texture.hpp"

namespace rtype::backend::raylib::renderer {

/// @brief The raylib textures of the renderer, by TextureId: a slot per texture, whose generation is bumped when it is
/// freed, so an id used after its texture was destroyed finds nothing instead of the slot's next texture.
class TextureTable {
  public:
    TextureTable() = default;
    /// @brief Frees every texture left, if the window (and its OpenGL context) is still open.
    ~TextureTable();
    TextureTable(const TextureTable&) = delete;
    TextureTable& operator=(const TextureTable&) = delete;
    TextureTable(TextureTable&&) = delete;
    TextureTable& operator=(TextureTable&&) = delete;

    /// @brief Uploads RGBA pixels as a texture, with the filter and wrap of @p desc.
    /// @throws std::invalid_argument If @p pixels is not desc.size.x * desc.size.y * 4 bytes, or the size is 0.
    [[nodiscard]] contract::renderer::TextureId create(const contract::renderer::TextureDesc& desc,
                                                       std::span<const std::byte> pixels);

    /// @brief Frees a texture; does nothing for an unknown or already destroyed id.
    void destroy(contract::renderer::TextureId id);

    /// @return The texture of @p id, or nullptr for an unknown or destroyed one.
    [[nodiscard]] const ::Texture2D* find(contract::renderer::TextureId id) const;

  private:
    std::vector<std::optional<::Texture2D>> _textures;  ///< By TextureId::index; empty: a free slot.
    std::vector<std::uint32_t> _generations;            ///< By TextureId::index.
    std::vector<std::uint32_t> _freeSlots;              ///< Slots to reuse.
};

}  // namespace rtype::backend::raylib::renderer
