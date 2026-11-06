/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Materials
*/

#pragma once

#include <cstddef>
#include <unordered_map>

#include "core/asset/AssetId.hpp"
#include "plugins/backend/graphics/Shader.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::backend::graphics {

/// @brief The materials ready to draw with, by asset, as the resource "rtype.Materials": what a drawing names
/// (MaterialId) and its shader's `instance uniform` defaults.
///
/// @details Part of the contract: the shader plugin fills it as it creates the materials in the renderer, and the
/// plugins that draw (sprites, texts, particles) read it without knowing the shader plugin. Each of them inserts it if
/// absent. A material not in it (unknown, not created yet, or without the shader plugin) is drawn with the default one.
/// @code
/// if (const Materials::Entry* material = materials->find(sprite.material)) {
///     drawn.material = material->material;
///     drawn.instanceValues = instance != nullptr ? instance->values : material->instanceDefaults;
/// }
/// @endcode
class Materials {
  public:
    /// @brief A material created in the renderer.
    struct Entry {
        MaterialId material;                      ///< What drawings name.
        ShaderInstanceValues instanceDefaults{};  ///< Its shader's `instance uniform` defaults.
    };

    Materials() = default;
    ~Materials() = default;
    Materials(const Materials& other) = delete;
    Materials& operator=(const Materials& other) = delete;
    Materials(Materials&& other) noexcept = default;
    Materials& operator=(Materials&& other) noexcept = default;

    /// @return How many materials are ready.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @return The material created for @p id (a MaterialAssets id), or nullptr if it is not ready: draw with the
    /// default material then.
    [[nodiscard]] const Entry* find(core::asset::AssetId id) const;

    /// @brief Makes @p id drawable with @p entry, replacing what it had.
    void set(core::asset::AssetId id, Entry entry);

    /// @brief Forgets @p id, once its material is destroyed; nothing if it is not there.
    void remove(core::asset::AssetId id);

  private:
    std::unordered_map<core::asset::AssetId, Entry> _entries;  ///< The ready materials, by asset.
};
RTYPE_ECS_RESOURCE(Materials, "rtype.Materials");

}  // namespace rtype::plugins::backend::graphics
