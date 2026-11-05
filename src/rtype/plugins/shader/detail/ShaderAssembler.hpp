/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderAssembler
*/

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "plugins/backend/graphics/Shader.hpp"
#include "plugins/shader/detail/ShaderParser.hpp"

namespace rtype::plugins::shader::detail {

/// @brief One stage of a target's shader program.
enum class ShaderStage : std::uint8_t {
    kVertex,
    kFragment,
};

/// @return The complete GLSL 450 of @p stage of @p shader drawn on @p target: the engine's prelude for that target
/// (the frame uniforms, the drawing's texture, the built-ins VERTEX, UV, COLOR, TEXTURE and TIME, the material block,
/// anonymous so its values are names of their own, its textures, and the instance values as globals), then `#line 1`
/// and the shader's body, then a main() that fills the built-ins as the default shader does and the instance values,
/// calls the shader's function and finishes the colour with the drawing's blend mode. Only TIME is a macro: a macro
/// would also replace a value's name after a '.'. Every stage keeps only its own function of the body, so code only one
/// stage allows (discard, derivatives) never reaches the other. The instance values are only carried when the shader
/// declares some: the GLSL then starts by defining RTYPE_INSTANCE_VALUES.
///
/// An empty shader (a default ParsedShader) leaves the prelude as it is, its `#pragma rtype_*` markers included,
/// which GLSL ignores: it is the default shader, which a backend's built-in shaders get by writing `#version 450` and
/// including the same prelude files.
///
/// The prelude is the contract every backend feeds (docs/Shaders.md): set 0 binding 0 the frame uniforms, set 1
/// binding 0 the drawing's texture, set 2 binding 0 the material block and bindings 1 to 3 its textures; sprites and
/// glyphs read SpriteInstance-like attributes (locations 0 to 6) and their instance values at locations 7 to 10;
/// shapes read ShapeInstance-like attributes (locations 0 to 7) and their instance values at locations 8 to 11;
/// particles read ParticleInstance attributes (locations 0 to 5) and a push constant block of their draw's
/// constants with the instance values at offset 48.
[[nodiscard]] std::string assembleShader(const ParsedShader& shader, backend::graphics::ShaderTarget target,
                                         ShaderStage stage);

/// @return @p body with the definition of `void <name>()` replaced by spaces, its line breaks kept; @p body as it is
/// if it does not define it.
[[nodiscard]] std::string blankFunction(std::string_view body, std::string_view name);

}  // namespace rtype::plugins::shader::detail
