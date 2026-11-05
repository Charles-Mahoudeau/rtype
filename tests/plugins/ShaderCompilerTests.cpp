/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderCompilerTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "plugins/backend/graphics/Shader.hpp"
#include "plugins/shader/detail/ShaderAssembler.hpp"
#include "plugins/shader/detail/ShaderCompiler.hpp"
#include "plugins/shader/detail/ShaderParser.hpp"
#include "plugins/shader/exceptions/ShaderExceptions.hpp"

using rtype::plugins::backend::graphics::CompiledShader;
using rtype::plugins::backend::graphics::ShaderTarget;
using rtype::plugins::shader::detail::assembleShader;
using rtype::plugins::shader::detail::compileShader;
using rtype::plugins::shader::detail::compileStage;
using rtype::plugins::shader::detail::ParsedShader;
using rtype::plugins::shader::detail::ShaderStage;
using rtype::plugins::shader::exceptions::ShaderException;

namespace {
/// @brief The first word of every SPIR-V module.
constexpr std::uint32_t kSpirvMagic = 0x07230203;

/// @brief A shader using every feature: material and instance values, a texture, both functions.
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
        static_cast<void>(compileShader(source, "test"));
    } catch (const ShaderException& error) {
        return error.what();
    }
    return {};
}
}  // namespace

TEST(ShaderCompiler, CompilesAShaderForEveryTarget) {
    const CompiledShader shader = compileShader(kFlash, "flash");
    ASSERT_EQ(shader.variants.size(), 4U);
    for (const ShaderTarget target : rtype::plugins::shader::detail::kAllTargets) {
        const auto* variant = shader.findVariant(target);
        ASSERT_NE(variant, nullptr);
        ASSERT_FALSE(variant->vertex.empty());
        ASSERT_FALSE(variant->fragment.empty());
        EXPECT_EQ(variant->vertex.front(), kSpirvMagic);
        EXPECT_EQ(variant->fragment.front(), kSpirvMagic);
    }
    EXPECT_NO_THROW(rtype::plugins::backend::graphics::validate(shader));
}

TEST(ShaderCompiler, CarriesWhatTheParserFound) {
    const CompiledShader shader = compileShader(kFlash, "flash");
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

TEST(ShaderCompiler, CompilesOnlyTheTargetsAskedFor) {
    const std::array targets{ShaderTarget::kParticle};
    const CompiledShader shader = compileShader("void fragment() {}\n", "minimal", targets);
    ASSERT_EQ(shader.variants.size(), 1U);
    EXPECT_EQ(shader.variants.front().target, ShaderTarget::kParticle);
}

TEST(ShaderCompiler, ReportsAnErrorAtTheShadersOwnLine) {
    const std::string message = errorOf("uniform float a;\n\nvoid fragment() {\n    COLOR = notDeclared;\n}\n");
    EXPECT_TRUE(message.starts_with("Shader 'test' (sprite fragment): line 4: 'notDeclared'")) << message;
}

TEST(ShaderCompiler, PassesTheParsersErrorsOn) {
    EXPECT_NE(errorOf("uniform mat4 m;\nvoid fragment() {}\n").find("line 1"), std::string::npos);
}

TEST(ShaderCompiler, RejectsInvalidGlslForAStage) {
    EXPECT_THROW(static_cast<void>(compileStage("#version 450\nvoid main() { oops }\n", ShaderStage::kFragment, "x")),
                 ShaderException);
}

TEST(ShaderCompiler, CompilesTheDefaultShaderOfEveryTarget) {
    for (const ShaderTarget target : rtype::plugins::shader::detail::kAllTargets) {
        for (const ShaderStage stage : {ShaderStage::kVertex, ShaderStage::kFragment}) {
            const std::vector<std::uint32_t> spirv =
                compileStage(assembleShader(ParsedShader{}, target, stage), stage, "default");
            ASSERT_FALSE(spirv.empty());
            EXPECT_EQ(spirv.front(), kSpirvMagic);
        }
    }
}
