/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderHandles
*/

#pragma once

#include <unordered_map>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/shader/Export.hpp"
#include "rtype/contract/renderer/IRenderer.hpp"
#include "rtype/contract/renderer/shaders/CompiledShader.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::graphics {
class TextureAssets;
}  // namespace rtype::plugins::graphics

namespace rtype::plugins::shader {

class MaterialAssets;
class ShaderAssets;

/// @brief The renderer's side of the shaders and materials, as the client resource "rtype.ShaderHandles" the
/// ShaderPlugin inserts: what each ShaderAssets and MaterialAssets entry was created as, for the drawing systems.
///
/// @details update() follows the two asset resources each frame: it creates the shaders compiled since the last frame,
/// creates or updates the materials added or changed once their shader and textures are ready, and destroys what was
/// unloaded. A material waits (and is drawn with the default material) while its shader is not created or a texture is
/// still queued; one whose values do not match its shader, or whose texture failed, is reported once and stays so until
/// it changes again. A texture unloaded while a material uses it must not be drawn: unload the material first.
class RTYPE_PLUGIN_SHADER_API ShaderHandles {
  public:
    /// @brief A material created in the renderer.
    struct RTYPE_PLUGIN_SHADER_API Material {
        contract::renderer::MaterialId material;                      ///< What drawings name.
        core::asset::AssetId shader;                                  ///< The shader it was created with.
        contract::renderer::ShaderInstanceValues instanceDefaults{};  ///< Its shader's `instance uniform` defaults.
    };

    ShaderHandles() = default;
    ~ShaderHandles() = default;
    ShaderHandles(const ShaderHandles& other) = delete;
    ShaderHandles& operator=(const ShaderHandles& other) = delete;
    ShaderHandles(ShaderHandles&& other) noexcept = default;
    ShaderHandles& operator=(ShaderHandles&& other) noexcept = default;

    /// @return The material @p id (a MaterialAssets id) was created as, or nullptr if it is not (yet): draw with the
    /// default material then.
    [[nodiscard]] const Material* getMaterial(core::asset::AssetId id) const;

    /// @return Whether the shader @p id (a ShaderAssets id) is created in the renderer.
    [[nodiscard]] bool hasShader(core::asset::AssetId id) const;

    /// @brief Brings the renderer up to date with @p shaders and @p materials, on the main thread, after @p textures
    /// were loaded for the frame. With a renderer that does not support shaders, only empties their queues.
    void update(ShaderAssets& shaders, MaterialAssets& materials, const graphics::TextureAssets& textures,
                contract::renderer::IRenderer& renderer);

  private:
    /// @brief Destroys the material @p id was created as, if any, and forgets it.
    void destroyMaterial(core::asset::AssetId id, contract::renderer::IRenderer& renderer);

    /// @brief Destroys the shader @p id was created as, and the materials created with it, which wait for a shader of
    /// this id again.
    void destroyShader(core::asset::AssetId id, contract::renderer::IRenderer& renderer);

    /// @brief Creates or updates the waiting material @p id.
    /// @return Whether it is done waiting: created, updated, unknown, or reported as wrong.
    bool createMaterial(core::asset::AssetId id, const ShaderAssets& shaders, const MaterialAssets& materials,
                        const graphics::TextureAssets& textures, contract::renderer::IRenderer& renderer);

    std::unordered_map<core::asset::AssetId, contract::renderer::ShaderId> _shaders;  ///< Created shaders, by asset.
    std::unordered_map<core::asset::AssetId, Material> _materials;                    ///< Created materials, by asset.
    std::vector<core::asset::AssetId> _waiting;  ///< Materials to create or update, once their shader is ready.
};
RTYPE_ECS_RESOURCE(ShaderHandles, "rtype.ShaderHandles");

}  // namespace rtype::plugins::shader
