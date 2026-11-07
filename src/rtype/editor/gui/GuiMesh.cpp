/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GuiMesh
*/

#include "GuiMesh.hpp"

#include <imgui.h>

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <span>
#include <vector>

#include "rtype/contract/renderer/IRenderer.hpp"
#include "rtype/contract/renderer/drawing/Mesh.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"

namespace rtype::editor::gui {

namespace {
constexpr unsigned int kGenerationShift = 32;  ///< Where the generation of a TextureId sits in an ImTextureID.
constexpr std::uint64_t kIndexMask = 0xFFFFFFFFULL;

/// @return The pixels of @p texture as RGBA: an alpha-only atlas is white with its coverage in alpha.
std::vector<std::byte> toRgba(ImTextureData& texture) {
    const auto count = static_cast<std::size_t>(texture.Width) * static_cast<std::size_t>(texture.Height);
    const auto* pixels = static_cast<const std::byte*>(texture.GetPixels());
    if (texture.Format == ImTextureFormat_RGBA32) {
        const std::span<const std::byte> all{pixels, count * 4};
        return {all.begin(), all.end()};
    }
    std::vector<std::byte> rgba(count * 4, std::byte{0xFF});
    std::size_t alphaOffset = 3;
    for (const std::byte alpha : std::span{pixels, count}) {
        rgba.at(alphaOffset) = alpha;
        alphaOffset += 4;
    }
    return rgba;
}

/// @brief Makes the renderer's texture of @p texture from all of its pixels, replacing the one it had.
void upload(ImTextureData& texture, contract::renderer::IRenderer& renderer) {
    if (texture.TexID != ImTextureID_Invalid) {
        renderer.destroy(fromImGuiTexture(texture.TexID));
    }
    const std::vector<std::byte> pixels = toRgba(texture);
    const contract::renderer::TextureDesc desc{
        .size = {static_cast<std::uint32_t>(texture.Width), static_cast<std::uint32_t>(texture.Height)},
        .filter = contract::renderer::TextureFilter::kLinear};
    texture.SetTexID(toImGuiTexture(renderer.createTexture(desc, pixels)));
    texture.SetStatus(ImTextureStatus_OK);
}
}  // namespace

std::uint64_t toImGuiTexture(contract::renderer::TextureId texture) noexcept {
    return (static_cast<std::uint64_t>(texture.generation) << kGenerationShift) | texture.index;
}

contract::renderer::TextureId fromImGuiTexture(std::uint64_t id) noexcept {
    return contract::renderer::TextureId{.index = static_cast<std::uint32_t>(id & kIndexMask),
                                         .generation = static_cast<std::uint32_t>(id >> kGenerationShift)};
}

void updateTextures(ImDrawData& drawData, contract::renderer::IRenderer& renderer) {
    if (drawData.Textures == nullptr) {
        return;
    }
    for (ImTextureData* texture : *drawData.Textures) {
        switch (texture->Status) {
            case ImTextureStatus_WantCreate:
            case ImTextureStatus_WantUpdates:
                // A grown atlas is sent whole: it changes a few times when the first panels open, then never.
                upload(*texture, renderer);
                break;
            case ImTextureStatus_WantDestroy:
                if (texture->TexID != ImTextureID_Invalid) {
                    renderer.destroy(fromImGuiTexture(texture->TexID));
                }
                texture->SetTexID(ImTextureID_Invalid);
                texture->SetStatus(ImTextureStatus_Destroyed);
                break;
            default:
                break;
        }
    }
}

contract::renderer::Mesh buildMesh(const ImDrawData& drawData) {
    contract::renderer::Mesh mesh;
    mesh.vertices.reserve(static_cast<std::size_t>(drawData.TotalVtxCount));
    mesh.indices.reserve(static_cast<std::size_t>(drawData.TotalIdxCount));
    const glm::vec2 scale{drawData.FramebufferScale.x, drawData.FramebufferScale.y};
    const glm::vec2 origin{drawData.DisplayPos.x, drawData.DisplayPos.y};
    for (const ImDrawList* list : drawData.CmdLists) {
        const auto firstVertex = static_cast<std::uint32_t>(mesh.vertices.size());
        for (const ImDrawVert& vertex :
             std::span{list->VtxBuffer.Data, static_cast<std::size_t>(list->VtxBuffer.Size)}) {
            mesh.vertices.push_back(contract::renderer::MeshVertex{
                .position = (glm::vec2{vertex.pos.x, vertex.pos.y} - origin) * scale,
                .uv = {vertex.uv.x, vertex.uv.y},
                .color = {(vertex.col >> IM_COL32_R_SHIFT) & 0xFFU, (vertex.col >> IM_COL32_G_SHIFT) & 0xFFU,
                          (vertex.col >> IM_COL32_B_SHIFT) & 0xFFU, (vertex.col >> IM_COL32_A_SHIFT) & 0xFFU}});
        }
        for (const ImDrawCmd& command : list->CmdBuffer) {
            if (command.UserCallback != nullptr || command.ElemCount == 0) {
                continue;
            }
            const auto firstIndex = static_cast<std::uint32_t>(mesh.indices.size());
            const std::uint32_t base = firstVertex + command.VtxOffset;
            const std::span<const ImDrawIdx> indices{list->IdxBuffer.Data,
                                                     static_cast<std::size_t>(list->IdxBuffer.Size)};
            for (const ImDrawIdx index : indices.subspan(command.IdxOffset, command.ElemCount)) {
                mesh.indices.push_back(base + index);
            }
            mesh.commands.push_back(contract::renderer::MeshCommand{
                .clipMin = (glm::vec2{command.ClipRect.x, command.ClipRect.y} - origin) * scale,
                .clipMax = (glm::vec2{command.ClipRect.z, command.ClipRect.w} - origin) * scale,
                .texture = fromImGuiTexture(command.GetTexID()),
                .indexOffset = firstIndex,
                .indexCount = command.ElemCount});
        }
    }
    return mesh;
}

}  // namespace rtype::editor::gui
