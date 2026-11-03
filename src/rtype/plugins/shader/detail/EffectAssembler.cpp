/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EffectAssembler
*/

#include "plugins/shader/detail/EffectAssembler.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>

#include "plugins/backend/graphics/Shader.hpp"
#include "plugins/shader/detail/EffectParser.hpp"

namespace rtype::plugins::shader::detail {

namespace {

using backend::graphics::ShaderTarget;
using backend::graphics::ShaderValueType;

// The prelude, in prelude/*.glsl, embedded by xmake's utils.bin2c rule: null-terminated, ASCII (the files are kept
// so, as they become chars).
constexpr auto kCommon = std::to_array<char>({
#include "common.glsl.h"
});
constexpr auto kParticleConstants = std::to_array<char>({
#include "particle_constants.glsl.h"
});
constexpr auto kFragmentCommon = std::to_array<char>({
#include "fragment_common.glsl.h"
});
constexpr auto kInstanceInput = std::to_array<char>({
#include "instance_input.glsl.h"
});
constexpr auto kSpriteVertex = std::to_array<char>({
#include "sprite.vert.glsl.h"
});
constexpr auto kParticleVertex = std::to_array<char>({
#include "particle.vert.glsl.h"
});
constexpr auto kSpriteFragment = std::to_array<char>({
#include "sprite.frag.glsl.h"
});
constexpr auto kGlyphFragment = std::to_array<char>({
#include "glyph.frag.glsl.h"
});

/// @brief Where a template takes the effect's declarations and body, and where its main() takes the instance values
/// and the effect's call.
constexpr std::string_view kEffectMarker = "#pragma rtype_effect";
constexpr std::string_view kMainMarker = "#pragma rtype_main";

/// @return The text of an embedded file, without its null terminator.
template <std::size_t N>
constexpr std::string_view text(const std::array<char, N>& file) noexcept {
    return {file.data(), N - 1};
}

/// @return @p glsl with its line @p marker replaced by @p replacement.
std::string replaceMarker(std::string glsl, std::string_view marker, std::string_view replacement) {
    const std::size_t at = glsl.find(marker);
    if (at != std::string::npos) {
        glsl.replace(at, marker.size(), replacement);
    }
    return glsl;
}

/// @return The GLSL type of @p type.
std::string_view glslType(ShaderValueType type) noexcept {
    switch (type) {
        case ShaderValueType::kVec2:
            return "vec2";
        case ShaderValueType::kVec3:
            return "vec3";
        case ShaderValueType::kVec4:
            return "vec4";
        case ShaderValueType::kInt:
            return "int";
        case ShaderValueType::kFloat:
            break;
    }
    return "float";
}

/// @return How the instance value @p value is read from @p source (4 vec4): its vec4 and components.
std::string instanceAccess(const backend::graphics::ShaderValue& value, std::string_view source) {
    constexpr std::string_view kComponents = "xyzw";
    const std::uint32_t slot = value.offset / 16;
    const std::uint32_t first = (value.offset % 16) / 4;
    std::uint32_t count = 1;
    switch (value.type) {
        case ShaderValueType::kVec2:
            count = 2;
            break;
        case ShaderValueType::kVec3:
            count = 3;
            break;
        case ShaderValueType::kVec4:
            count = 4;
            break;
        case ShaderValueType::kFloat:
        case ShaderValueType::kInt:
            break;
    }
    std::string access = std::format("{}[{}].{}", source, slot, kComponents.substr(first, count));
    return value.type == ShaderValueType::kInt ? std::format("floatBitsToInt({})", access) : access;
}

/// @return The material block, its textures and the instance values, each under its own name. The material block is
/// anonymous, so its members are names of their own, and the instance values are plain globals main() fills: no macro,
/// which would also replace the name after a '.' (a member, a swizzle).
std::string declareValues(const ParsedEffect& effect) {
    std::string glsl = "\n// The effect's values.\n";
    if (!effect.materialValues.empty()) {
        glsl += "layout(set = 2, binding = 0, std140) uniform RtypeMaterial {\n";
        for (const DeclaredValue& declared : effect.materialValues) {
            glsl += std::format("    layout(offset = {}) {} {};\n", declared.value.offset,
                                glslType(declared.value.type), declared.value.name);
        }
        glsl += "};\n";
    }
    for (const backend::graphics::ShaderTexture& texture : effect.textures) {
        glsl += std::format("layout(set = 2, binding = {}) uniform sampler2D {};\n", texture.slot + 1, texture.name);
    }
    for (const DeclaredValue& declared : effect.instanceValues) {
        glsl += std::format("{} {};\n", glslType(declared.value.type), declared.value.name);
    }
    return glsl;
}

/// @return The statements that fill the instance values from @p source (4 vec4), for main().
std::string loadInstanceValues(const ParsedEffect& effect, std::string_view source) {
    std::string glsl;
    for (const DeclaredValue& declared : effect.instanceValues) {
        glsl += std::format("    {} = {};\n", declared.value.name, instanceAccess(declared.value, source));
    }
    return glsl;
}

}  // namespace

std::string blankFunction(std::string_view body, std::string_view name) {
    std::string blanked{body};
    const std::string signature = std::format("void {}", name);
    for (std::size_t at = blanked.find(signature); at != std::string::npos; at = blanked.find(signature, at + 1)) {
        const std::size_t after = at + signature.size();
        const std::size_t parenthesis = blanked.find_first_not_of(" \t\r\n", after);
        if (parenthesis == std::string::npos || blanked.at(parenthesis) != '(') {
            continue;
        }
        const std::size_t brace = blanked.find('{', parenthesis);
        if (brace == std::string::npos) {
            return blanked;
        }
        std::size_t depth = 0;
        std::size_t end = brace;
        for (; end < blanked.size(); ++end) {
            if (blanked.at(end) == '{') {
                ++depth;
            } else if (blanked.at(end) == '}' && --depth == 0) {
                break;
            }
        }
        for (std::size_t i = at; i <= end && i < blanked.size(); ++i) {
            if (blanked.at(i) != '\n') {
                blanked.at(i) = ' ';
            }
        }
        return blanked;
    }
    return blanked;
}

std::string assembleEffect(const ParsedEffect& effect, ShaderTarget target, ShaderStage stage) {
    const bool particle = target == ShaderTarget::kParticle;
    const bool vertex = stage == ShaderStage::kVertex;
    std::string glsl{text(kCommon)};
    if (particle) {
        glsl += text(kParticleConstants);
    }
    std::string_view stageTemplate;
    if (vertex) {
        stageTemplate = particle ? text(kParticleVertex) : text(kSpriteVertex);
    } else {
        glsl += text(kFragmentCommon);
        if (!particle) {
            glsl += text(kInstanceInput);
        }
        stageTemplate = target == ShaderTarget::kGlyph ? text(kGlyphFragment) : text(kSpriteFragment);
    }
    const std::string effectCode = std::format("{}\n#line 1\n{}\n", declareValues(effect),
                                               blankFunction(effect.body, vertex ? "fragment" : "vertex"));
    const bool calls = vertex ? effect.hasVertex : effect.hasFragment;
    const std::string_view call = vertex ? "    vertex();" : "    fragment();";
    const std::string mainCode =
        std::format("{}{}", loadInstanceValues(effect, particle ? "rtypeParticles.instance" : "rtypeInstance"),
                    calls ? call : std::string_view{});
    glsl += replaceMarker(replaceMarker(std::string{stageTemplate}, kEffectMarker, effectCode), kMainMarker, mainCode);
    return glsl;
}

}  // namespace rtype::plugins::shader::detail
