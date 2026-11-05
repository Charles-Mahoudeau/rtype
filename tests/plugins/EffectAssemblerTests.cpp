/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EffectAssemblerTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

#include "plugins/backend/graphics/Shader.hpp"
#include "plugins/shader/detail/EffectAssembler.hpp"
#include "plugins/shader/detail/EffectParser.hpp"

using rtype::plugins::backend::graphics::ShaderTarget;
using rtype::plugins::shader::detail::assembleEffect;
using rtype::plugins::shader::detail::blankFunction;
using rtype::plugins::shader::detail::ParsedEffect;
using rtype::plugins::shader::detail::parseEffect;
using rtype::plugins::shader::detail::ShaderStage;

namespace {
/// @brief An effect with material values, instance values, a texture, a helper and both functions.
constexpr std::string_view kEffect =
    "uniform vec4 tint = vec4(1.0);\n"
    "instance uniform float flash;\n"
    "instance uniform int seed;\n"
    "uniform sampler2D noise;\n"
    "float wave(float x) { return sin(x + TIME); }\n"
    "void vertex() { VERTEX.y += wave(VERTEX.x); }\n"
    "void fragment() { if (texture(noise, UV).r < 0.1) { discard; } COLOR.rgb = mix(COLOR.rgb, tint.rgb, flash); }\n";

/// @return The GLSL of @p stage of kEffect for @p target.
std::string assemble(ShaderTarget target, ShaderStage stage, std::string_view source = kEffect) {
    return assembleEffect(parseEffect(source, "test"), target, stage);
}

/// @return Whether @p text contains @p part.
bool has(std::string_view text, std::string_view part) { return text.contains(part); }
}  // namespace

TEST(EffectAssembler, BlanksOnlyTheNamedFunctionAndKeepsItsLines) {
    const std::string body = "void fragmentHelper() {}\nvoid fragment() {\n    discard;\n}\nvoid vertex() {}\n";
    const std::string blanked = blankFunction(body, "fragment");
    EXPECT_TRUE(has(blanked, "void fragmentHelper() {}"));
    EXPECT_FALSE(has(blanked, "discard"));
    EXPECT_TRUE(has(blanked, "void vertex() {}"));
    EXPECT_EQ(std::ranges::count(blanked, '\n'), std::ranges::count(body, '\n'));
    EXPECT_EQ(blankFunction(body, "missing"), body);
}

TEST(EffectAssembler, StartsWithThePreludeThenTheGameLinesFromOne) {
    const std::string glsl = assemble(ShaderTarget::kSprite, ShaderStage::kFragment);
    // #version comes before any code (only comments may precede it).
    ASSERT_NE(glsl.find("#version 450\n"), std::string::npos);
    EXPECT_LT(glsl.find("#version 450\n"), glsl.find("layout("));
    EXPECT_FALSE(glsl.contains("#pragma rtype_"));  // every marker was replaced
    const std::size_t line = glsl.find("\n#line 1\n");
    ASSERT_NE(line, std::string::npos);
    EXPECT_LT(glsl.find("uniform sampler2D TEXTURE"), line);
    EXPECT_GT(glsl.find("float wave(float x)"), line);
}

TEST(EffectAssembler, DeclaresTheMaterialAsAnAnonymousBlockAndItsTextures) {
    const std::string glsl = assemble(ShaderTarget::kSprite, ShaderStage::kFragment);
    EXPECT_TRUE(has(glsl,
                    "layout(set = 2, binding = 0, std140) uniform RtypeMaterial {\n"
                    "    layout(offset = 0) vec4 tint;\n};"));
    EXPECT_TRUE(has(glsl, "layout(set = 2, binding = 1) uniform sampler2D noise;"));
    EXPECT_FALSE(has(glsl, "#define tint"));  // no macro: it would replace `.tint` members and swizzles too
}

TEST(EffectAssembler, FillsTheInstanceValuesFromTheirSourceBeforeTheEffect) {
    const std::string sprite = assemble(ShaderTarget::kSprite, ShaderStage::kFragment);
    EXPECT_TRUE(has(sprite, "float flash;\n"));
    EXPECT_TRUE(has(sprite,
                    "    flash = rtypeInstance[0].x;\n    seed = floatBitsToInt(rtypeInstance[0].y);\n"
                    "    fragment();\n"));
    const std::string particle = assemble(ShaderTarget::kParticle, ShaderStage::kVertex);
    EXPECT_TRUE(has(particle, "    flash = rtypeParticles.instance[0].x;\n"));
    EXPECT_TRUE(has(particle, "vec4 instance[4];"));
}

TEST(EffectAssembler, EachStageKeepsOnlyItsOwnFunction) {
    const std::string vertex = assemble(ShaderTarget::kSprite, ShaderStage::kVertex);
    EXPECT_FALSE(has(vertex, "discard"));
    EXPECT_TRUE(has(vertex, "    vertex();\n"));
    EXPECT_TRUE(has(vertex, "float wave(float x)"));  // helpers stay in both stages
    const std::string fragment = assemble(ShaderTarget::kSprite, ShaderStage::kFragment);
    EXPECT_FALSE(has(fragment, "VERTEX.y += wave"));
    EXPECT_TRUE(has(fragment, "    fragment();\n"));
}

TEST(EffectAssembler, CallsOnlyTheFunctionsTheEffectDefines) {
    const std::string vertex = assemble(ShaderTarget::kSprite, ShaderStage::kVertex, "void fragment() {}\n");
    EXPECT_FALSE(has(vertex, "    vertex();\n"));
}

TEST(EffectAssembler, StartsEachTargetFromItsDefaultColour) {
    EXPECT_TRUE(
        has(assemble(ShaderTarget::kSprite, ShaderStage::kFragment), "COLOR = texture(TEXTURE, UV) * rtypeColorIn;"));
    EXPECT_TRUE(has(assemble(ShaderTarget::kGlyph, ShaderStage::kFragment),
                    "COLOR = rtypeColorIn * rtypeCoverage(rtypeUvWidth);"));
    EXPECT_TRUE(
        has(assemble(ShaderTarget::kParticle, ShaderStage::kFragment), "COLOR = texture(TEXTURE, UV) * rtypeColorIn;"));
}

TEST(EffectAssembler, ReadsEachTargetsOwnVertexInputs) {
    EXPECT_TRUE(
        has(assemble(ShaderTarget::kSprite, ShaderStage::kVertex), "layout(location = 7) in vec4 rtypeInstance[4];"));
    const std::string particle = assemble(ShaderTarget::kParticle, ShaderStage::kVertex);
    EXPECT_TRUE(has(particle, "layout(location = 5) in float rtypeLife;"));
    EXPECT_FALSE(has(particle, "rtypeInstance"));
}

TEST(EffectAssembler, CarriesTheInstanceValuesOnlyWhenTheEffectHasSome) {
    EXPECT_TRUE(assemble(ShaderTarget::kSprite, ShaderStage::kVertex)
                    .starts_with("#version 450\n#define RTYPE_INSTANCE_VALUES\n"));
    const std::string vertex = assemble(ShaderTarget::kSprite, ShaderStage::kVertex, "void fragment() {}\n");
    EXPECT_TRUE(vertex.starts_with("#version 450\n"));
    EXPECT_FALSE(has(vertex, "#define RTYPE_INSTANCE_VALUES"));
}

TEST(EffectAssembler, LeavesThePreludeAsItIsForTheDefaultEffect) {
    for (const ShaderTarget target : {ShaderTarget::kSprite, ShaderTarget::kGlyph, ShaderTarget::kParticle}) {
        for (const ShaderStage stage : {ShaderStage::kVertex, ShaderStage::kFragment}) {
            const std::string glsl = assembleEffect(ParsedEffect{}, target, stage);
            EXPECT_TRUE(glsl.starts_with("#version 450\n"));
            // The markers stay, as in the built-in shaders that #include the prelude: GLSL ignores them.
            EXPECT_TRUE(has(glsl, "#pragma rtype_effect"));
            EXPECT_TRUE(has(glsl, "#pragma rtype_main"));
            EXPECT_FALSE(has(glsl, "#line"));
            EXPECT_FALSE(has(glsl, "#define RTYPE_INSTANCE_VALUES"));
            EXPECT_FALSE(has(glsl, "vertex();"));
            EXPECT_FALSE(has(glsl, "fragment();"));
        }
    }
}
