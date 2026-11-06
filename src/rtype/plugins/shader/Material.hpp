/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Material
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <string>
#include <variant>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/shader/Export.hpp"
#include "rtype/contract/renderer/shaders/CompiledShader.hpp"

namespace rtype::plugins::shader {

/// @brief A value of a material, of one of the types a shader declares: float, vec2, vec3, vec4 or int.
using MaterialValue = std::variant<float, glm::vec2, glm::vec3, glm::vec4, std::int32_t>;

/// @brief A shader and the values of its `uniform`s and textures, by name: what a game describes, shared by every
/// drawing that uses it. The values it does not set keep the shader's defaults.
/// @code
/// rtype::plugins::shader::Material hit{.shader = shaders->load("assets/shaders/flash.glsl")};
/// hit.set("tint", glm::vec4{1.0F, 0.2F, 0.2F, 1.0F}).setTexture("noise", textures->load("assets/noise.png"));
/// @endcode
struct RTYPE_PLUGIN_SHADER_API Material {
    /// @brief One value set by name.
    struct Value {
        std::string name;     ///< As declared in the shader.
        MaterialValue value;  ///< Its type must be the declared one.
    };

    /// @brief One texture set by name.
    struct Texture {
        std::string name;              ///< As declared in the shader (`uniform sampler2D <name>;`).
        core::asset::AssetId texture;  ///< From TextureAssets::load().
    };

    core::asset::AssetId shader;    ///< From ShaderAssets::load() or add().
    std::vector<Value> values;      ///< The values set; a name set twice keeps the last.
    std::vector<Texture> textures;  ///< The textures set; a name set twice keeps the last.

    /// @brief Sets the value @p name, replacing a previous one.
    /// @return This material, to chain.
    Material& set(const std::string& name, MaterialValue value);

    /// @brief Sets the texture @p name, replacing a previous one.
    /// @return This material, to chain.
    Material& setTexture(const std::string& name, core::asset::AssetId texture);
};

/// @return The material block of @p material for @p shader: the shader's defaults, with every value @p material sets
/// written at its offset. CompiledShader::materialSize bytes.
/// @throws exceptions::ShaderException If a value has no `uniform` of this name in the shader (an `instance uniform`
/// is set per drawing, not in a material) or is not of its type.
[[nodiscard]] RTYPE_PLUGIN_SHADER_API std::vector<std::byte> packMaterialValues(
    const contract::renderer::CompiledShader& shader, const Material& material);

/// @brief Writes @p value at the place of @p shader's `instance uniform` @p name in @p values.
/// @throws exceptions::ShaderException If @p shader has no `instance uniform` of this name (a `uniform` is set in a
/// material) or @p value is not of its type.
RTYPE_PLUGIN_SHADER_API void writeInstanceValue(const contract::renderer::CompiledShader& shader,
                                                contract::renderer::ShaderInstanceValues& values,
                                                const std::string& name, const MaterialValue& value);

/// @return The textures of @p material for @p shader, by slot; an unset slot keeps a default AssetId.
/// @throws exceptions::ShaderException If a texture has no `uniform sampler2D` of this name in the shader.
[[nodiscard]] RTYPE_PLUGIN_SHADER_API std::array<core::asset::AssetId, contract::renderer::kShaderMaxTextures>
resolveMaterialTextures(const contract::renderer::CompiledShader& shader, const Material& material);

}  // namespace rtype::plugins::shader
