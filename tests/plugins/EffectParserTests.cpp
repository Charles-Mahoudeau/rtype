/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EffectParserTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <glm/ext/vector_float4.hpp>
#include <string>
#include <string_view>

#include "plugins/backend/graphics/Shader.hpp"
#include "plugins/shader/detail/EffectParser.hpp"
#include "plugins/shader/exceptions/ShaderExceptions.hpp"

using rtype::plugins::backend::graphics::ShaderValueType;
using rtype::plugins::shader::detail::ParsedEffect;
using rtype::plugins::shader::detail::parseEffect;
using rtype::plugins::shader::exceptions::ShaderException;

namespace {
/// @return The float at @p offset of @p block.
float readFloat(const ParsedEffect& effect, std::uint32_t offset) {
    float value = 0.0F;
    std::memcpy(&value, effect.materialDefaults.data() + offset, sizeof(value));
    return value;
}

/// @return The message of the ShaderException parsing @p source throws, or an empty string if it throws none.
std::string errorOf(std::string_view source) {
    try {
        static_cast<void>(parseEffect(source, "test"));
    } catch (const ShaderException& error) {
        return error.what();
    }
    return {};
}
}  // namespace

TEST(EffectParser, ReadsMaterialValuesWithTheirLayoutAndDefaults) {
    const ParsedEffect effect = parseEffect(
        "uniform float flash = 0.5;\nuniform vec4 tint = vec4(1.0, 0.5, 0.25, 1.0);\nuniform vec2 scroll;\n"
        "void fragment() {}",
        "flash");
    ASSERT_EQ(effect.materialValues.size(), 3U);
    EXPECT_EQ(effect.materialValues.at(0).value.name, "flash");
    EXPECT_EQ(effect.materialValues.at(0).value.offset, 0U);
    EXPECT_EQ(effect.materialValues.at(1).value.type, ShaderValueType::kVec4);
    EXPECT_EQ(effect.materialValues.at(1).value.offset, 16U);  // a vec4 is aligned on 16 bytes
    EXPECT_EQ(effect.materialValues.at(2).value.offset, 32U);
    EXPECT_EQ(effect.materialValues.at(2).line, 3U);
    EXPECT_EQ(effect.materialSize, 48U);  // 40 bytes, rounded up to 16
    EXPECT_FLOAT_EQ(readFloat(effect, 0), 0.5F);
    EXPECT_FLOAT_EQ(readFloat(effect, 20), 0.5F);
    EXPECT_FLOAT_EQ(readFloat(effect, 32), 0.0F);
}

TEST(EffectParser, PacksAFloatAfterAVec3) {
    const ParsedEffect effect = parseEffect("uniform vec3 color;\nuniform float alpha;\nvoid fragment() {}", "x");
    EXPECT_EQ(effect.materialValues.at(1).value.offset, 12U);
    EXPECT_EQ(effect.materialSize, 16U);
}

TEST(EffectParser, ReadsInstanceValuesIntoTheirOwnBlock) {
    const ParsedEffect effect = parseEffect(
        "instance uniform float flash = 1.0;\ninstance uniform vec4 tint = vec4(0.5);\nvoid fragment() {}", "x");
    ASSERT_EQ(effect.instanceValues.size(), 2U);
    EXPECT_EQ(effect.instanceValues.at(1).value.offset, 16U);
    EXPECT_TRUE(effect.materialValues.empty());
    EXPECT_EQ(effect.materialSize, 0U);
    EXPECT_FLOAT_EQ(effect.instanceDefaults.at(0).x, 1.0F);
    EXPECT_EQ(effect.instanceDefaults.at(1), glm::vec4{0.5F});
}

TEST(EffectParser, ReadsTexturesInSlotOrder) {
    const ParsedEffect effect =
        parseEffect("uniform sampler2D noise;\nuniform sampler2D palette;\nvoid fragment() {}", "x");
    ASSERT_EQ(effect.textures.size(), 2U);
    EXPECT_EQ(effect.textures.at(1).name, "palette");
    EXPECT_EQ(effect.textures.at(1).slot, 1U);
}

TEST(EffectParser, StoresAnIntDefaultAsAnInt) {
    const ParsedEffect effect = parseEffect("uniform int frames = 4;\nvoid fragment() {}", "x");
    std::int32_t value = 0;
    std::memcpy(&value, effect.materialDefaults.data(), sizeof(value));
    EXPECT_EQ(value, 4);
}

TEST(EffectParser, KeepsTheLinesOfTheSourceAndBlanksTheDeclarations) {
    const ParsedEffect effect = parseEffect("uniform float a; // tint\n\nvoid fragment() {\n    COLOR *= a;\n}\n", "x");
    EXPECT_EQ(effect.body, "\n\nvoid fragment() {\n    COLOR *= a;\n}\n");
}

TEST(EffectParser, FindsTheVertexAndFragmentFunctions) {
    const ParsedEffect both = parseEffect("void vertex () {}\nvoid\nfragment( ) {}", "x");
    EXPECT_TRUE(both.hasVertex);
    EXPECT_TRUE(both.hasFragment);
    const ParsedEffect commented = parseEffect("// void vertex() {}\nvoid fragment() {}", "x");
    EXPECT_FALSE(commented.hasVertex);
}

TEST(EffectParser, IgnoresDeclarationsInBlockComments) {
    const ParsedEffect effect = parseEffect("/*\nuniform float a;\n*/\nvoid fragment() {}", "x");
    EXPECT_TRUE(effect.materialValues.empty());
}

TEST(EffectParser, NamesTheEffectAndTheLineOfAnError) {
    EXPECT_EQ(errorOf("void fragment() {}\nuniform mat4 m;"),
              "Effect 'test', line 2: type 'mat4' is not float, vec2, vec3, vec4, int or sampler2D");
}

TEST(EffectParser, RejectsWhatItCannotRead) {
    EXPECT_NE(errorOf("uniform float a\nvoid fragment() {}"), "");                    // no ';'
    EXPECT_NE(errorOf("uniform float a; uniform float b;\nvoid fragment() {}"), "");  // two on a line
    EXPECT_NE(errorOf("uniform float a[4];\nvoid fragment() {}"), "");                // an array
    EXPECT_NE(errorOf("uniform vec3 a = vec3(1, 2);\nvoid fragment() {}"), "");       // two numbers for three
    EXPECT_NE(errorOf("uniform float a = pi;\nvoid fragment() {}"), "");              // not a number
    EXPECT_NE(errorOf("instance uniform sampler2D t;\nvoid fragment() {}"), "");      // an instance texture
}

TEST(EffectParser, RejectsReservedAndDuplicateNames) {
    EXPECT_NE(errorOf("uniform vec4 COLOR;\nvoid fragment() {}").find("reserved"), std::string::npos);
    EXPECT_NE(errorOf("uniform float rtypeX;\nvoid fragment() {}").find("reserved"), std::string::npos);
    EXPECT_NE(errorOf("uniform float a;\ninstance uniform float a;\nvoid fragment() {}").find("twice"),
              std::string::npos);
}

TEST(EffectParser, RejectsInstanceValuesPastTheBudgetAndTooManyTextures) {
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

TEST(EffectParser, RejectsAnEffectWithoutFunction) {
    EXPECT_NE(errorOf("uniform float a;").find("neither"), std::string::npos);
}
