/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MaterialAssets
*/

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "core/asset/AssetStore.hpp"
#include "plugins/shader/Export.hpp"
#include "plugins/shader/Material.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::shader {

/// @brief The game's materials, as the ECS resource "rtype.MaterialAssets" the ShaderPlugin inserts: SpriteRenderer
/// and the other drawings keep the AssetId of the one they are drawn with.
///
/// @details Built in code for now (a loader for material files can come as for particle effects). The ShaderPlugin
/// creates each material in the renderer once its shader and textures are ready, and updates it when setValue() or
/// setTexture() change it, from the next frame drawn.
/// @code
/// rtype::plugins::shader::Material hit{.shader = shaders->load("assets/shaders/flash.glsl")};
/// const rtype::core::asset::AssetId material = materials->add("enemy_hit", hit.set("tint", glm::vec4{1.0F}));
/// @endcode
class RTYPE_PLUGIN_SHADER_API MaterialAssets {
  public:
    /// @brief One material: the name it was added under, and the material.
    using Entry = core::asset::AssetStore<Material>::Entry;

    MaterialAssets() = default;
    ~MaterialAssets() = default;
    MaterialAssets(const MaterialAssets& other) = delete;
    MaterialAssets& operator=(const MaterialAssets& other) = delete;
    MaterialAssets(MaterialAssets&& other) noexcept = default;
    MaterialAssets& operator=(MaterialAssets&& other) noexcept = default;

    /// @return The number of materials known.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @brief Adds @p material, known by @p name. Its values are checked against its shader when the ShaderPlugin
    /// creates it: a value the shader does not declare is reported then.
    /// @return Its id (AssetId::fromPath(name)). Adding a name twice keeps the first material.
    /// @throws core::asset::exceptions::AssetLoadException If another name already has this id (a hash collision).
    core::asset::AssetId add(const std::string& name, Material material);

    /// @return The material @p id names, or nullptr if it is unknown.
    [[nodiscard]] const Material* get(core::asset::AssetId id) const;

    /// @return The entry of @p id, or nullptr if it is unknown.
    [[nodiscard]] const Entry* getEntry(core::asset::AssetId id) const;

    /// @brief Changes the value @p name of the material @p id: every drawing using it sees it from the next frame.
    /// @return False, doing nothing, if no material has this id.
    bool setValue(core::asset::AssetId id, const std::string& name, MaterialValue value);

    /// @brief Changes the texture @p name of the material @p id, as setValue().
    /// @return False, doing nothing, if no material has this id.
    bool setTexture(core::asset::AssetId id, const std::string& name, core::asset::AssetId texture);

    /// @brief Forgets the material @p id names; the ShaderPlugin destroys it in the renderer.
    /// @return False, doing nothing, if no material has this id.
    bool unload(core::asset::AssetId id);

    /// @return The materials added or changed since the last call, each once, for the ShaderPlugin.
    [[nodiscard]] std::vector<core::asset::AssetId> takeChanged();

    /// @return The materials unloaded since the last call, for the ShaderPlugin to destroy.
    [[nodiscard]] std::vector<core::asset::AssetId> takeUnloaded();

  private:
    /// @brief Records that @p id changed, once.
    void markChanged(core::asset::AssetId id);

    core::asset::AssetStore<Material> _materials{"Material"};  ///< Every material, by id.
    std::vector<core::asset::AssetId> _changed;                ///< Added or changed since the last takeChanged().
    std::vector<core::asset::AssetId> _unloaded;               ///< Unloaded since the last takeUnloaded().
};
RTYPE_ECS_RESOURCE(MaterialAssets, "rtype.MaterialAssets");

}  // namespace rtype::plugins::shader
