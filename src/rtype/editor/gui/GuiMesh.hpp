/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GuiMesh
*/

#pragma once

#include <cstdint>

#include "editor/Export.hpp"
#include "rtype/contract/renderer/IRenderer.hpp"
#include "rtype/contract/renderer/drawing/Mesh.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"

struct ImDrawData;

namespace rtype::editor::gui {

/// @return The id Dear ImGui keeps for a texture of the renderer (its ImTextureID).
[[nodiscard]] RTYPE_EDITOR_API std::uint64_t toImGuiTexture(contract::renderer::TextureId texture) noexcept;

/// @return The texture of the renderer an id of Dear ImGui stands for.
[[nodiscard]] RTYPE_EDITOR_API contract::renderer::TextureId fromImGuiTexture(std::uint64_t id) noexcept;

/// @brief Creates, updates and destroys the textures Dear ImGui asks for (its font atlas), in the renderer. Call it
/// with the frame's draw data before buildMesh(), which reads the ids it sets.
/// @throws std::invalid_argument If the renderer refuses a texture.
RTYPE_EDITOR_API void updateTextures(ImDrawData& drawData, contract::renderer::IRenderer& renderer);

/// @return The triangles of the frame's draw data, in framebuffer pixels: ready for IRenderer::drawMesh().
[[nodiscard]] RTYPE_EDITOR_API contract::renderer::Mesh buildMesh(const ImDrawData& drawData);

}  // namespace rtype::editor::gui
