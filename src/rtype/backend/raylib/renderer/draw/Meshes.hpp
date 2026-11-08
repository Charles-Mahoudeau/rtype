/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Meshes
*/

#pragma once

#include "backend/raylib/renderer/TextureTable.hpp"
#include "rtype/contract/renderer/drawing/Mesh.hpp"

namespace rtype::backend::raylib::renderer::draw {

/// @brief Draws a tool's @p mesh in framebuffer pixels through rlgl, raylib's immediate-mode layer: each command's
/// triangles with its texture (plain colours without one), clipped to its rectangle. Indices out of range are skipped.
void drawMesh(const contract::renderer::Mesh& mesh, const TextureTable& textures);

}  // namespace rtype::backend::raylib::renderer::draw
