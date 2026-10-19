/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextureAssets
*/

#pragma once

#include <cstddef>
#include <filesystem>
#include <glm/ext/vector_uint2.hpp>
#include <span>
#include <string>

#include "plugins/backend/graphics/Assets.hpp"
#include "plugins/backend/graphics/IRenderer.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::backend {
/// @brief What is known about a loaded texture.
struct TextureInfo {
    glm::uvec2 size{0};  ///< In pixels.
};

/// @brief How a texture loaded from an image is sampled.
struct TextureOptions {
    graphics::TextureFilter filter{graphics::TextureFilter::kNearest};  ///< Sharp pixels by default: pixel art.
    bool repeat{false};  ///< Tiles the texture when the source rect goes past its edges.
};

/// @brief Textures are created and destroyed by the renderer.
template <>
struct AssetTraits<graphics::TextureId> {
    using Info = TextureInfo;           ///< The texture's size.
    using Owner = graphics::IRenderer;  ///< Creates and destroys textures.
};

/// @brief The loaded textures, the resource "rtype.TextureAssets" the BackendPlugin inserts: systems load and find
/// textures by path, and SpriteRenderer components keep only their TextureId.
/// @code
/// void loadShip(rtype::ecs::ResMut<rtype::plugins::backend::TextureAssets> textures,
///               rtype::ecs::NonSendMut<rtype::plugins::backend::Backend> backend) {
///     const auto ship = rtype::plugins::backend::loadTexture(*textures, *backend->renderer, "assets/ship.png");
/// }
/// @endcode
using TextureAssets = Assets<graphics::TextureId>;

RTYPE_ECS_RESOURCE(Assets<graphics::TextureId>, "rtype.TextureAssets");

/// @brief Loads an image file as a texture, or returns the texture already loaded from this path.
/// @details Decodes the file (PNG, JPEG, BMP, TGA... through stb_image) into RGBA pixels, creates the texture in the
/// renderer and records it in `assets` under the path. Call it where the renderer may be used: on the main thread.
/// @return The texture's id.
/// @throws exceptions::AssetLoadException If the file cannot be read or decoded.
[[nodiscard]] graphics::TextureId loadTexture(TextureAssets& assets, graphics::IRenderer& renderer,
                                              const std::filesystem::path& path, const TextureOptions& options = {});

/// @brief Loads an image already in memory as a texture, or returns the texture already recorded under this name.
/// @details Same as loading a file, for images that do not come from the disk (embedded, downloaded).
/// @param encoded The image file's bytes (PNG, JPEG...).
/// @return The texture's id.
/// @throws exceptions::AssetLoadException If the bytes cannot be decoded.
[[nodiscard]] graphics::TextureId loadTexture(TextureAssets& assets, graphics::IRenderer& renderer,
                                              const std::string& name, std::span<const std::byte> encoded,
                                              const TextureOptions& options = {});
}  // namespace rtype::plugins::backend
