/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderParser
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "plugins/backend/graphics/Shader.hpp"

namespace rtype::plugins::shader::detail {

/// @brief One value a shader declares, laid out in its block, with its default value.
struct DeclaredValue {
    backend::graphics::ShaderValue value;  ///< Name, type and offset in its block.
    std::uint32_t line{0};                 ///< Line of the declaration in the shader, from 1.
};

/// @brief A shader's source once its declarations are read: what the engine generates around it, and the GLSL the
/// game wrote, the declarations blanked out.
struct ParsedShader {
    std::string name;                                            ///< For messages: its file, or its name.
    std::vector<DeclaredValue> materialValues;                   ///< `uniform` values, in declaration order.
    std::uint32_t materialSize{0};                               ///< Bytes of the material block, a multiple of 16.
    std::vector<std::byte> materialDefaults;                     ///< The material block filled with the defaults.
    std::vector<DeclaredValue> instanceValues;                   ///< `instance uniform` values, in declaration order.
    backend::graphics::ShaderInstanceValues instanceDefaults{};  ///< The instance values' defaults.
    std::vector<backend::graphics::ShaderTexture> textures;      ///< `uniform sampler2D`, slots in declaration order.
    bool hasVertex{false};                                       ///< Whether it defines `void vertex()`.
    bool hasFragment{false};                                     ///< Whether it defines `void fragment()`.
    std::string body;  ///< The source, every declaration line left empty, so line numbers stay the game's.
};

/// @brief Reads the declarations of a shader, written in the engine's GLSL dialect (docs/Shaders.md), one per line:
/// @code{.glsl}
/// uniform vec4 tint = vec4(1.0, 0.5, 0.5, 1.0);  // in the material, shared by the drawings that use it
/// instance uniform float flash = 0.0;            // in each drawing, kShaderInstanceBytes for all of them
/// uniform sampler2D noise;                       // an extra texture of the material, kShaderMaxTextures at most
/// void fragment() { COLOR.rgb = mix(COLOR.rgb, tint.rgb, flash); }
/// @endcode
/// Values are float, vec2, vec3, vec4 or int, laid out as std140 and std430 lay them out (the same for these types),
/// their default a number or a `vecN(...)` of numbers (one number fills every component), 0 when omitted.
/// @param name Names the shader in messages.
/// @throws exceptions::ShaderException Naming the shader and the line: an unknown type, a declaration it cannot read
/// (an array, a block, several on a line), a name declared twice or reserved by the engine, a default it cannot read,
/// instance values past kShaderInstanceBytes, too many textures, or neither vertex() nor fragment().
[[nodiscard]] ParsedShader parseShader(std::string_view source, std::string_view name);

}  // namespace rtype::plugins::shader::detail
