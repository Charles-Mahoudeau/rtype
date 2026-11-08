/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Meshes
*/

#include "Meshes.hpp"

#include <raylib.h>
#include <rlgl.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "backend/raylib/renderer/TextureTable.hpp"
#include "rtype/contract/renderer/drawing/Mesh.hpp"

namespace rtype::backend::raylib::renderer::draw {
namespace {

/// @brief Sends the triangles of one command.
void drawCommand(const contract::renderer::Mesh& mesh, const contract::renderer::MeshCommand& command) {
    const std::size_t end = std::min<std::size_t>(command.indexOffset + command.indexCount, mesh.indices.size());
    rlBegin(RL_TRIANGLES);
    for (std::size_t index = command.indexOffset; index < end; ++index) {
        const std::uint32_t vertexIndex = mesh.indices.at(index);
        if (vertexIndex >= mesh.vertices.size()) {
            continue;
        }
        const contract::renderer::MeshVertex& vertex = mesh.vertices.at(vertexIndex);
        rlColor4ub(vertex.color.x, vertex.color.y, vertex.color.z, vertex.color.w);
        rlTexCoord2f(vertex.uv.x, vertex.uv.y);
        rlVertex2f(vertex.position.x, vertex.position.y);
    }
    rlEnd();
}

}  // namespace

void drawMesh(const contract::renderer::Mesh& mesh, const TextureTable& textures) {
    for (const contract::renderer::MeshCommand& command : mesh.commands) {
        BeginScissorMode(static_cast<int>(command.clipMin.x), static_cast<int>(command.clipMin.y),
                         static_cast<int>(command.clipMax.x - command.clipMin.x),
                         static_cast<int>(command.clipMax.y - command.clipMin.y));
        const ::Texture2D* texture = textures.find(command.texture);
        rlSetTexture(texture != nullptr ? texture->id : rlGetTextureIdDefault());
        drawCommand(mesh, command);
        rlSetTexture(0);
        EndScissorMode();
    }
}

}  // namespace rtype::backend::raylib::renderer::draw
