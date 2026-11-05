/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderCompiler
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "plugins/backend/graphics/Shader.hpp"
#include "plugins/shader/detail/ShaderAssembler.hpp"

namespace rtype::plugins::shader::detail {

/// @brief Every target a shader is drawn on.
inline constexpr std::array<backend::graphics::ShaderTarget, backend::graphics::kShaderTargetCount> kAllTargets{
    backend::graphics::ShaderTarget::kSprite, backend::graphics::ShaderTarget::kGlyph,
    backend::graphics::ShaderTarget::kParticle, backend::graphics::ShaderTarget::kShape};
static_assert(
    [] {
        std::array<bool, backend::graphics::kShaderTargetCount> seen{};
        for (const backend::graphics::ShaderTarget target : kAllTargets) {
            bool& found = seen.at(static_cast<std::size_t>(target));
            if (found) {
                return false;
            }
            found = true;
        }
        return true;
    }(),
    "kAllTargets must list every ShaderTarget once");

/// @return The SPIR-V 1.6 (Vulkan semantics: the engine's intermediate format, which every backend uses or translates)
/// of @p glsl, a complete GLSL 450 @p stage (assembleShader()).
/// @param what Names the shader in messages: the shader, its target and stage.
/// @throws exceptions::ShaderException If glslang rejects it: its errors, each with its line in the shader.
[[nodiscard]] std::vector<std::uint32_t> compileStage(std::string_view glsl, ShaderStage stage, std::string_view what);

/// @return @p source, a shader in the engine's dialect, compiled for @p targets: their SPIR-V, and what the parser
/// found (the material and instance values with their defaults, the textures). Ready for IRenderer::createShader():
/// it passes backend::graphics::validate().
/// @details glslang's process-wide state is set up by the first compilation, once, and torn down at exit: glslang
/// wants it exactly once per process.
/// @param name Names the shader in messages: its file, or its name.
/// @throws exceptions::ShaderException If the parser cannot read a declaration or glslang rejects a stage.
[[nodiscard]] backend::graphics::CompiledShader compileShader(
    std::string_view source, std::string_view name,
    std::span<const backend::graphics::ShaderTarget> targets = kAllTargets);

}  // namespace rtype::plugins::shader::detail
