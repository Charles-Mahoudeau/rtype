/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FrameStats
*/

#pragma once

#include <cstdint>

namespace rtype::contract::renderer {

/// @brief What the last frame the renderer ended drew, for a tool that shows it (IRenderer::getFrameStats()).
struct FrameStats {
    std::uint32_t sprites{0};        ///< Sprites and glyphs drawn in the scene.
    std::uint32_t shapes{0};         ///< Shapes drawn in the scene.
    std::uint32_t overlayItems{0};   ///< Drawings of the overlay (IRenderer::drawOverlay()).
    std::uint32_t meshTriangles{0};  ///< Triangles of the meshes (IRenderer::drawMesh()).
    std::uint32_t textures{0};       ///< Textures alive.
};

}  // namespace rtype::contract::renderer
