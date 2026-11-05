/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MaterialInstance
*/

#pragma once

#include <string>

#include "plugins/backend/graphics/Shader.hpp"
#include "plugins/shader/Material.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::plugins::shader {

/// @brief An entity's own `instance uniform` values, as the component "rtype.MaterialInstance": its SpriteRenderer is
/// drawn with them instead of its material's shader's defaults. Not reflected.
///
/// @details The values are the 64 bytes of a drawing's graphics::ShaderInstanceValues, laid out by the shader: start
/// from of() (the shader's defaults) and set() them by name, from the shader of the entity's material.
/// @code
/// const rtype::plugins::backend::graphics::CompiledShader& flash = *shaders->get(flashShader);
/// commands.entity(enemy).add(rtype::plugins::shader::MaterialInstance::of(flash).set(flash, "amount", 1.0F));
/// @endcode
struct MaterialInstance {
    backend::graphics::ShaderInstanceValues values{};  ///< As the shader lays them out.

    /// @return An instance with @p shader's defaults.
    [[nodiscard]] static MaterialInstance of(const backend::graphics::CompiledShader& shader);

    /// @brief Sets @p shader's `instance uniform` @p name.
    /// @return This instance, to chain.
    /// @throws exceptions::ShaderException If @p shader has no `instance uniform` of this name, or @p value is not of
    /// its type.
    MaterialInstance& set(const backend::graphics::CompiledShader& shader, const std::string& name,
                          const MaterialValue& value);
};
RTYPE_ECS_COMPONENT(MaterialInstance, "rtype.MaterialInstance");

}  // namespace rtype::plugins::shader
