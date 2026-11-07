/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Mesh
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint4_sized.hpp>
#include <vector>

#include "rtype/contract/renderer/drawing/Texture.hpp"

namespace rtype::contract::renderer {

/// @brief One vertex of a Mesh: 20 bytes, tightly packed.
struct MeshVertex {
    glm::vec2 position{0.0F};  ///< In framebuffer pixels, origin at the top-left corner, Y down.
    glm::vec2 uv{0.0F};        ///< Texture coordinates, in [0, 1] over the texture.
    glm::u8vec4 color{255};    ///< RGBA, 0 to 255, in sRGB and not premultiplied: multiplied by the texture.
};

/// @brief A run of a Mesh's indices drawn with one texture inside one clip rectangle.
struct MeshCommand {
    glm::vec2 clipMin{0.0F};       ///< Top-left corner of the clip, in framebuffer pixels.
    glm::vec2 clipMax{0.0F};       ///< Bottom-right corner of the clip, in framebuffer pixels.
    TextureId texture;             ///< The texture the fragments are multiplied by; none: plain colours.
    std::uint32_t indexOffset{0};  ///< First index of the run in Mesh::indices.
    std::uint32_t indexCount{0};   ///< Number of indices of the run: a multiple of 3, one triangle per 3.
};

/// @brief Indexed triangles in framebuffer pixels, drawn on top of the whole frame by IRenderer::drawMesh(): what a
/// tool (a debug UI, an editor) hands the renderer without the renderer knowing the library that made them.
///
/// @details Commands are drawn in order, each one clipped to its rectangle and blended over what is below with
/// premultiplied alpha. A texture is drawn as a premultiplied sRGB texture is: its colour is multiplied by the vertex
/// colour, and a font atlas that is white with its coverage in alpha gives text of the vertex colour.
struct Mesh {
    std::vector<MeshVertex> vertices;    ///< Every vertex of the mesh.
    std::vector<std::uint32_t> indices;  ///< Indices into vertices, three per triangle.
    std::vector<MeshCommand> commands;   ///< The runs of indices, in draw order.
};

}  // namespace rtype::contract::renderer
