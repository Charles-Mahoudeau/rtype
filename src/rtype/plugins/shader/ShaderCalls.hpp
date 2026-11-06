/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderCalls
*/

#pragma once

#include <cstdint>
#include <string_view>

#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/call/CallTraits.hpp"
#include "rtype/ecs/type/String.hpp"

namespace rtype::plugins::shader {

inline constexpr std::string_view kLoadShaderCall = "rtype.shader.load";            ///< ShaderAssets::load().
inline constexpr std::string_view kAddMaterialCall = "rtype.shader.addMaterial";    ///< MaterialAssets::add().
inline constexpr std::string_view kSetValueCall = "rtype.shader.setMaterialValue";  ///< MaterialAssets::setValue().
inline constexpr std::string_view kSetTextureCall =
    "rtype.shader.setMaterialTexture";  ///< MaterialAssets::setTexture().

/// @brief The arguments of `rtype.shader.load`: reads and compiles a shader file.
struct LoadShaderArguments {
    ecs::String path;  ///< The file, relative to the game directory.
};
RTYPE_ECS_CALL_DATA(LoadShaderArguments, "rtype.shader.LoadShaderArguments", path);

/// @brief The result of `rtype.shader.load`.
struct ShaderResult {
    core::asset::AssetId shader;  ///< The shader's id, for `rtype.shader.addMaterial`.
};
RTYPE_ECS_CALL_DATA(ShaderResult, "rtype.shader.ShaderResult", shader);

/// @brief The arguments of `rtype.shader.addMaterial`: a material of @ref shader, with the shader's default values.
struct AddMaterialArguments {
    ecs::String name;             ///< What the material is known by.
    core::asset::AssetId shader;  ///< Its shader, from `rtype.shader.load`.
};
RTYPE_ECS_CALL_DATA(AddMaterialArguments, "rtype.shader.AddMaterialArguments", name, shader);

/// @brief The result of `rtype.shader.addMaterial`.
struct MaterialResult {
    core::asset::AssetId material;  ///< The material's id, for a SpriteRenderer and the calls below.
};
RTYPE_ECS_CALL_DATA(MaterialResult, "rtype.shader.MaterialResult", material);

/// @brief The arguments of `rtype.shader.setMaterialValue`: a float, or a vector of @ref size components.
struct SetMaterialValueArguments {
    core::asset::AssetId material;  ///< The material, from `rtype.shader.addMaterial`.
    ecs::String name;               ///< The `uniform`, as declared in the shader.
    std::uint32_t size{1};          ///< 1: a float (or a whole number, for an `int`); 2 to 4: a vec2 to a vec4.
    float x{0.0F};                  ///< The float, or the first component.
    float y{0.0F};                  ///< The second component.
    float z{0.0F};                  ///< The third component.
    float w{0.0F};                  ///< The fourth component.
};
RTYPE_ECS_CALL_DATA(SetMaterialValueArguments, "rtype.shader.SetMaterialValueArguments", material, name, size, x, y, z,
                    w);

/// @brief The arguments of `rtype.shader.setMaterialTexture`.
struct SetMaterialTextureArguments {
    core::asset::AssetId material;  ///< The material, from `rtype.shader.addMaterial`.
    ecs::String name;               ///< The `uniform sampler2D`, as declared in the shader.
    core::asset::AssetId texture;   ///< The texture, from `rtype.backend.loadTexture`.
};
RTYPE_ECS_CALL_DATA(SetMaterialTextureArguments, "rtype.shader.SetMaterialTextureArguments", material, name, texture);

/// @brief Registers the shader engine calls on @p app, for scripts: `rtype.shader.load`, `rtype.shader.addMaterial`,
/// `rtype.shader.setMaterialValue` and `rtype.shader.setMaterialTexture`. They throw a ShaderException naming what is
/// wrong (an unknown material, a size that is not 1 to 4). Called by the ShaderPlugin.
void addShaderCalls(ecs::App& app);

/// @brief Registers the setup forms of the plugin on @p registry: its library does it on every side, so a server gets
/// the ids of what it does not load, and so does building the plugin, for a program without libraries.
void addShaderForms(ecs::PluginRegistry& registry);

}  // namespace rtype::plugins::shader
