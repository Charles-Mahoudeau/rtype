/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Material
*/

#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/backend/graphics/MaterialValue.hpp"
#include "plugins/backend/graphics/Shader.hpp"

namespace rtype::plugins::shader {

/// @brief A value of a material, of one of the types a shader declares (the contract's MaterialValue).
using MaterialValue = backend::graphics::MaterialValue;

/// @brief Writes a value at the place of a shader's `instance uniform` (the contract's writeInstanceValue()).
using backend::graphics::writeInstanceValue;

/// @brief A shader and the values of its `uniform`s and textures, by name: what a game describes, shared by every
/// drawing that uses it. The values it does not set keep the shader's defaults.
/// @code
/// rtype::plugins::shader::Material hit{.shader = shaders->load("assets/shaders/flash.glsl")};
/// hit.set("tint", glm::vec4{1.0F, 0.2F, 0.2F, 1.0F}).setTexture("noise", textures->load("assets/noise.png"));
/// @endcode
struct Material {
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
[[nodiscard]] std::vector<std::byte> packMaterialValues(const backend::graphics::CompiledShader& shader,
                                                        const Material& material);

/// @return The textures of @p material for @p shader, by slot; an unset slot keeps a default AssetId.
/// @throws exceptions::ShaderException If a texture has no `uniform sampler2D` of this name in the shader.
[[nodiscard]] std::array<core::asset::AssetId, backend::graphics::kShaderMaxTextures> resolveMaterialTextures(
    const backend::graphics::CompiledShader& shader, const Material& material);

}  // namespace rtype::plugins::shader
