/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderCompiler
*/

#include "plugins/shader/detail/ShaderCompiler.hpp"

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "plugins/shader/detail/ShaderAssembler.hpp"
#include "plugins/shader/detail/ShaderParser.hpp"
#include "plugins/shader/exceptions/ShaderExceptions.hpp"
#include "rtype/contract/renderer/shaders/CompiledShader.hpp"

namespace rtype::plugins::shader::detail {

namespace {

/// @brief glslang's process-wide state: InitializeProcess() when the first shader is compiled, FinalizeProcess() at
/// exit. A function-local static, so it is set up once, even from several threads.
class GlslangProcess {
  public:
    GlslangProcess() { glslang::InitializeProcess(); }
    ~GlslangProcess() { glslang::FinalizeProcess(); }
    GlslangProcess(const GlslangProcess& other) = delete;
    GlslangProcess& operator=(const GlslangProcess& other) = delete;
    GlslangProcess(GlslangProcess&& other) = delete;
    GlslangProcess& operator=(GlslangProcess&& other) = delete;

    /// @brief Sets glslang up, the first time only.
    static void ensure() { static const GlslangProcess process; }
};

/// @return @p log, glslang's messages, as the lines of the shader: "ERROR: 0:4: x" becomes "line 4: x", and its
/// summary lines ("2 compilation errors.") are dropped.
std::string formatLog(std::string_view log) {
    std::string formatted;
    for (std::size_t start = 0; start < log.size();) {
        const std::size_t end = std::min(log.find('\n', start), log.size());
        std::string_view line = log.substr(start, end - start);
        start = end + 1;
        for (const std::string_view prefix : {std::string_view{"ERROR: 0:"}, std::string_view{"WARNING: 0:"}}) {
            if (line.starts_with(prefix)) {
                line.remove_prefix(prefix.size());
                formatted += std::format("{}line {}\n", formatted.empty() ? "" : "  ", line);
            }
        }
    }
    if (!formatted.empty()) {
        formatted.pop_back();
    }
    return formatted.empty() ? std::string{log} : formatted;
}

/// @return The glslang stage of @p stage.
EShLanguage toGlslang(ShaderStage stage) noexcept {
    return stage == ShaderStage::kVertex ? EShLangVertex : EShLangFragment;
}

/// @return The name of @p target, for messages.
std::string_view nameOf(contract::renderer::ShaderTarget target) noexcept {
    switch (target) {
        case contract::renderer::ShaderTarget::kGlyph:
            return "glyph";
        case contract::renderer::ShaderTarget::kParticle:
            return "particle";
        case contract::renderer::ShaderTarget::kShape:
            return "shape";
        case contract::renderer::ShaderTarget::kSprite:
            break;
    }
    return "sprite";
}

}  // namespace

std::vector<std::uint32_t> compileStage(std::string_view glsl, ShaderStage stage, std::string_view what) {
    GlslangProcess::ensure();
    const EShLanguage language = toGlslang(stage);
    glslang::TShader shader{language};
    const std::string source{glsl};
    const char* text = source.c_str();
    shader.setStrings(&text, 1);
    // SPIR-V with Vulkan semantics (sets, bindings, push constants) is the engine's intermediate format, not a tie to
    // the Vulkan backend: Vulkan uses it as is, Metal, OpenGL and Direct3D backends translate it (SPIRV-Cross, as
    // MoltenVK does). The latest version (1.6, Vulkan 1.3), the one the engine's renderer targets.
    shader.setEnvInput(glslang::EShSourceGlsl, language, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);
    // glslang takes OR-ed flags as an EShMessages, as its own glslangValidator does.
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    const auto messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
    if (!shader.parse(GetDefaultResources(), 450, false, messages)) {
        throw exceptions::ShaderException{std::format("{}: {}", what, formatLog(shader.getInfoLog()))};
    }
    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(messages)) {
        throw exceptions::ShaderException{std::format("{}: {}", what, formatLog(program.getInfoLog()))};
    }
    std::vector<unsigned int> spirv;
    glslang::GlslangToSpv(*program.getIntermediate(language), spirv);
    return {spirv.begin(), spirv.end()};
}

contract::renderer::CompiledShader compileShader(std::string_view source, std::string_view name,
                                                 std::span<const contract::renderer::ShaderTarget> targets) {
    const ParsedShader parsed = parseShader(source, name);
    contract::renderer::CompiledShader shader{.name = std::string{name},
                                              .materialSize = parsed.materialSize,
                                              .materialDefaults = parsed.materialDefaults,
                                              .instanceDefaults = parsed.instanceDefaults,
                                              .textures = parsed.textures};
    for (const DeclaredValue& declared : parsed.materialValues) {
        shader.materialValues.push_back(declared.value);
    }
    for (const DeclaredValue& declared : parsed.instanceValues) {
        shader.instanceValues.push_back(declared.value);
    }
    for (const contract::renderer::ShaderTarget target : targets) {
        const auto compile = [&](ShaderStage stage) {
            const std::string what = std::format("Shader '{}' ({} {})", name, nameOf(target),
                                                 stage == ShaderStage::kVertex ? "vertex" : "fragment");
            return compileStage(assembleShader(parsed, target, stage), stage, what);
        };
        shader.variants.push_back(contract::renderer::ShaderVariant{
            .target = target, .vertex = compile(ShaderStage::kVertex), .fragment = compile(ShaderStage::kFragment)});
    }
    return shader;
}

}  // namespace rtype::plugins::shader::detail
