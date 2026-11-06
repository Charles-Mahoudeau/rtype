/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderParserTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <glm/ext/vector_float4.hpp>
#include <string>
#include <string_view>

#include "plugins/shader/detail/ShaderParser.hpp"
#include "plugins/shader/exceptions/ShaderExceptions.hpp"
#include "rtype/contract/renderer/shaders/CompiledShader.hpp"

using rtype::contract::renderer::ShaderValueType;
using rtype::plugins::shader::detail::ParsedShader;
using rtype::plugins::shader::detail::parseShader;
using rtype::plugins::shader::exceptions::ShaderException;

namespace {
/// @return The float at @p offset of @p block.
float readFloat(const ParsedShader& shader, std::uint32_t offset) {
    float value = 0.0F;
    std::memcpy(&value, shader.materialDefaults.data() + offset, sizeof(value));
    return value;
}

/// @return The message of the ShaderException parsing @p source throws, or an empty string if it throws none.
std::string errorOf(std::string_view source) {
    try {
        static_cast<void>(parseShader(source, "test"));
    } catch (const ShaderException& error) {
        return error.what();
    }
    return {};
}
}  // namespace

TEST(ShaderParser, ReadsMaterialValuesWithTheirLayoutAndDefaults) {
    const ParsedShader shader = parseShader(
        "uniform float flash = 0.5;\nuniform vec4 tint = vec4(1.0, 0.5, 0.25, 1.0);\nuniform vec2 scroll;\n"
        "void fragment() {}",
        "flash");
    ASSERT_EQ(shader.materialValues.size(), 3U);
    EXPECT_EQ(shader.materialValues.at(0).value.name, "flash");
    EXPECT_EQ(shader.materialValues.at(0).value.offset, 0U);
    EXPECT_EQ(shader.materialValues.at(1).value.type, ShaderValueType::kVec4);
    EXPECT_EQ(shader.materialValues.at(1).value.offset, 16U);  // a vec4 is aligned on 16 bytes
    EXPECT_EQ(shader.materialValues.at(2).value.offset, 32U);
    EXPECT_EQ(shader.materialValues.at(2).line, 3U);
    EXPECT_EQ(shader.materialSize, 48U);  // 40 bytes, rounded up to 16
    EXPECT_FLOAT_EQ(readFloat(shader, 0), 0.5F);
    EXPECT_FLOAT_EQ(readFloat(shader, 20), 0.5F);
    EXPECT_FLOAT_EQ(readFloat(shader, 32), 0.0F);
}

TEST(ShaderParser, PacksAFloatAfterAVec3) {
    const ParsedShader shader = parseShader("uniform vec3 color;\nuniform float alpha;\nvoid fragment() {}", "x");
    EXPECT_EQ(shader.materialValues.at(1).value.offset, 12U);
    EXPECT_EQ(shader.materialSize, 16U);
}

TEST(ShaderParser, ReadsInstanceValuesIntoTheirOwnBlock) {
    const ParsedShader shader = parseShader(
        "instance uniform float flash = 1.0;\ninstance uniform vec4 tint = vec4(0.5);\nvoid fragment() {}", "x");
    ASSERT_EQ(shader.instanceValues.size(), 2U);
    EXPECT_EQ(shader.instanceValues.at(1).value.offset, 16U);
    EXPECT_TRUE(shader.materialValues.empty());
    EXPECT_EQ(shader.materialSize, 0U);
    EXPECT_FLOAT_EQ(shader.instanceDefaults.at(0).x, 1.0F);
    EXPECT_EQ(shader.instanceDefaults.at(1), glm::vec4{0.5F});
}

TEST(ShaderParser, ReadsTexturesInSlotOrder) {
    const ParsedShader shader =
        parseShader("uniform sampler2D noise;\nuniform sampler2D palette;\nvoid fragment() {}", "x");
    ASSERT_EQ(shader.textures.size(), 2U);
    EXPECT_EQ(shader.textures.at(1).name, "palette");
    EXPECT_EQ(shader.textures.at(1).slot, 1U);
}

TEST(ShaderParser, StoresAnIntDefaultAsAnInt) {
    const ParsedShader shader = parseShader("uniform int frames = 4;\nvoid fragment() {}", "x");
    std::int32_t value = 0;
    std::memcpy(&value, shader.materialDefaults.data(), sizeof(value));
    EXPECT_EQ(value, 4);
}

TEST(ShaderParser, KeepsTheLinesOfTheSourceAndBlanksTheDeclarations) {
    const ParsedShader shader = parseShader("uniform float a; // tint\n\nvoid fragment() {\n    COLOR *= a;\n}\n", "x");
    EXPECT_EQ(shader.body, "\n\nvoid fragment() {\n    COLOR *= a;\n}\n");
}

TEST(ShaderParser, FindsTheVertexAndFragmentFunctions) {
    const ParsedShader both = parseShader("void vertex () {}\nvoid\nfragment( ) {}", "x");
    EXPECT_TRUE(both.hasVertex);
    EXPECT_TRUE(both.hasFragment);
    const ParsedShader commented = parseShader("// void vertex() {}\nvoid fragment() {}", "x");
    EXPECT_FALSE(commented.hasVertex);
}

TEST(ShaderParser, IgnoresDeclarationsInBlockComments) {
    const ParsedShader shader = parseShader("/*\nuniform float a;\n*/\nvoid fragment() {}", "x");
    EXPECT_TRUE(shader.materialValues.empty());
}

TEST(ShaderParser, NamesTheShaderAndTheLineOfAnError) {
    EXPECT_EQ(errorOf("void fragment() {}\nuniform mat4 m;"),
              "Shader 'test', line 2: type 'mat4' is not float, vec2, vec3, vec4, int or sampler2D");
}

TEST(ShaderParser, RejectsWhatItCannotRead) {
    EXPECT_NE(errorOf("uniform float a\nvoid fragment() {}"), "");                    // no ';'
    EXPECT_NE(errorOf("uniform float a; uniform float b;\nvoid fragment() {}"), "");  // two on a line
    EXPECT_NE(errorOf("uniform float a[4];\nvoid fragment() {}"), "");                // an array
    EXPECT_NE(errorOf("uniform vec3 a = vec3(1, 2);\nvoid fragment() {}"), "");       // two numbers for three
    EXPECT_NE(errorOf("uniform float a = pi;\nvoid fragment() {}"), "");              // not a number
    EXPECT_NE(errorOf("instance uniform sampler2D t;\nvoid fragment() {}"), "");      // an instance texture
}

TEST(ShaderParser, RejectsReservedAndDuplicateNames) {
    EXPECT_NE(errorOf("uniform vec4 COLOR;\nvoid fragment() {}").find("reserved"), std::string::npos);
    EXPECT_NE(errorOf("uniform float rtypeX;\nvoid fragment() {}").find("reserved"), std::string::npos);
    EXPECT_NE(errorOf("uniform float a;\ninstance uniform float a;\nvoid fragment() {}").find("twice"),
              std::string::npos);
}

TEST(ShaderParser, RejectsInstanceValuesPastTheBudgetAndTooManyTextures) {
    std::string instances;
    for (int i = 0; i < 5; ++i) {
        instances += "instance uniform vec4 v" + std::to_string(i) + ";\n";
    }
    EXPECT_NE(errorOf(instances + "void fragment() {}").find("64 bytes"), std::string::npos);
    EXPECT_NE(errorOf("uniform sampler2D a;\nuniform sampler2D b;\nuniform sampler2D c;\nuniform sampler2D d;\n"
                      "void fragment() {}")
                  .find("more than 3 textures"),
              std::string::npos);
}

TEST(ShaderParser, RejectsAShaderWithoutFunction) {
    EXPECT_NE(errorOf("uniform float a;").find("neither"), std::string::npos);
}
