/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EffectCompilerTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "plugins/backend/graphics/Shader.hpp"
#include "plugins/shader/detail/EffectAssembler.hpp"
#include "plugins/shader/detail/EffectCompiler.hpp"
#include "plugins/shader/detail/EffectParser.hpp"
#include "plugins/shader/exceptions/ShaderExceptions.hpp"

using rtype::plugins::backend::graphics::CompiledShader;
using rtype::plugins::backend::graphics::ShaderTarget;
using rtype::plugins::shader::detail::assembleEffect;
using rtype::plugins::shader::detail::compileEffect;
using rtype::plugins::shader::detail::compileStage;
using rtype::plugins::shader::detail::ParsedEffect;
using rtype::plugins::shader::detail::ShaderStage;
using rtype::plugins::shader::exceptions::ShaderException;

namespace {
/// @brief The first word of every SPIR-V module.
constexpr std::uint32_t kSpirvMagic = 0x07230203;

/// @brief An effect using every feature: material and instance values, a texture, both functions.
constexpr std::string_view kFlash =
    "uniform vec4 tint = vec4(1.0, 0.2, 0.2, 1.0);\n"
    "instance uniform float flash;\n"
    "uniform sampler2D noise;\n"
    "void vertex() { VERTEX.y += sin(TIME); }\n"
    "void fragment() {\n"
    "    if (texture(noise, UV).r < 0.05) { discard; }\n"
    "    COLOR.rgb = mix(COLOR.rgb, tint.rgb * COLOR.a, flash);\n"
    "}\n";

/// @return The message of the ShaderException compiling @p source throws, or an empty string if it throws none.
std::string errorOf(std::string_view source) {
    try {
        static_cast<void>(compileEffect(source, "test"));
    } catch (const ShaderException& error) {
        return error.what();
    }
    return {};
}
}  // namespace

TEST(EffectCompiler, CompilesAnEffectForEveryTarget) {
    const CompiledShader shader = compileEffect(kFlash, "flash");
    ASSERT_EQ(shader.variants.size(), 3U);
    for (const ShaderTarget target : {ShaderTarget::kSprite, ShaderTarget::kGlyph, ShaderTarget::kParticle}) {
        const auto* variant = shader.findVariant(target);
        ASSERT_NE(variant, nullptr);
        ASSERT_FALSE(variant->vertex.empty());
        ASSERT_FALSE(variant->fragment.empty());
        EXPECT_EQ(variant->vertex.front(), kSpirvMagic);
        EXPECT_EQ(variant->fragment.front(), kSpirvMagic);
    }
    EXPECT_NO_THROW(rtype::plugins::backend::graphics::validate(shader));
}

TEST(EffectCompiler, CarriesWhatTheParserFound) {
    const CompiledShader shader = compileEffect(kFlash, "flash");
    EXPECT_EQ(shader.name, "flash");
    ASSERT_EQ(shader.materialValues.size(), 1U);
    EXPECT_EQ(shader.materialValues.front().name, "tint");
    EXPECT_EQ(shader.materialSize, 16U);
    EXPECT_EQ(shader.materialDefaults.size(), 16U);
    ASSERT_EQ(shader.instanceValues.size(), 1U);
    EXPECT_EQ(shader.instanceValues.front().name, "flash");
    ASSERT_EQ(shader.textures.size(), 1U);
    EXPECT_EQ(shader.textures.front().name, "noise");
}

TEST(EffectCompiler, CompilesOnlyTheTargetsAskedFor) {
    const std::array targets{ShaderTarget::kParticle};
    const CompiledShader shader = compileEffect("void fragment() {}\n", "minimal", targets);
    ASSERT_EQ(shader.variants.size(), 1U);
    EXPECT_EQ(shader.variants.front().target, ShaderTarget::kParticle);
}

TEST(EffectCompiler, ReportsAnErrorAtTheEffectsOwnLine) {
    const std::string message = errorOf("uniform float a;\n\nvoid fragment() {\n    COLOR = notDeclared;\n}\n");
    EXPECT_TRUE(message.starts_with("Effect 'test' (sprite fragment): line 4: 'notDeclared'")) << message;
}

TEST(EffectCompiler, PassesTheParsersErrorsOn) {
    EXPECT_NE(errorOf("uniform mat4 m;\nvoid fragment() {}\n").find("line 1"), std::string::npos);
}

TEST(EffectCompiler, RejectsInvalidGlslForAStage) {
    EXPECT_THROW(static_cast<void>(compileStage("#version 450\nvoid main() { oops }\n", ShaderStage::kFragment, "x")),
                 ShaderException);
}

TEST(EffectCompiler, CompilesTheDefaultEffectOfEveryTarget) {
    for (const ShaderTarget target : {ShaderTarget::kSprite, ShaderTarget::kGlyph, ShaderTarget::kParticle}) {
        for (const ShaderStage stage : {ShaderStage::kVertex, ShaderStage::kFragment}) {
            const std::vector<std::uint32_t> spirv =
                compileStage(assembleEffect(ParsedEffect{}, target, stage), stage, "default");
            ASSERT_FALSE(spirv.empty());
            EXPECT_EQ(spirv.front(), kSpirvMagic);
        }
    }
}
