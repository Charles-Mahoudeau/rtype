/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderAssetsTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/shader/ShaderAssets.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

using rtype::core::asset::AssetId;
using rtype::plugins::shader::ShaderAssets;

static_assert(rtype::ecs::Resource<ShaderAssets>);

namespace {
/// @brief A minimal valid shader.
constexpr std::string_view kTint = "uniform vec4 tint = vec4(1.0);\nvoid fragment() { COLOR *= tint; }\n";

/// @brief A shader glslang rejects.
constexpr std::string_view kBroken = "void fragment() { COLOR = notDeclared; }\n";

/// @brief A text file in a temporary directory of its own, removed when the test ends.
class TemporaryFile {
  public:
    TemporaryFile(const std::string& name, std::string_view text)
        : _directory{std::filesystem::temp_directory_path() /
                     std::format("rtype_shader_assets_{:08x}", std::random_device{}())},
          _path{_directory / name} {
        std::filesystem::create_directories(_directory);
        std::ofstream{_path, std::ios::binary} << text;
    }
    ~TemporaryFile() {
        std::error_code error;
        std::filesystem::remove_all(_directory, error);
    }
    TemporaryFile(const TemporaryFile& other) = delete;
    TemporaryFile& operator=(const TemporaryFile& other) = delete;
    TemporaryFile(TemporaryFile&& other) = delete;
    TemporaryFile& operator=(TemporaryFile&& other) = delete;

    [[nodiscard]] const std::filesystem::path& getPath() const noexcept { return _path; }

  private:
    std::filesystem::path _directory;  ///< Removed with the file.
    std::filesystem::path _path;       ///< The file.
};
}  // namespace

TEST(ShaderAssets, LoadsAndCompilesAShaderFile) {
    const TemporaryFile file{"tint.glsl", kTint};
    ShaderAssets shaders;
    const AssetId id = shaders.load(file.getPath());
    EXPECT_EQ(id, AssetId::fromPath(file.getPath()));
    ASSERT_NE(shaders.get(id), nullptr);
    EXPECT_EQ(shaders.get(id)->variants.size(), 4U);
    EXPECT_EQ(shaders.takeAdded(), (std::vector<AssetId>{id}));
    EXPECT_TRUE(shaders.takeAdded().empty());
}

TEST(ShaderAssets, LoadingTwiceCompilesOnce) {
    const TemporaryFile file{"tint.glsl", kTint};
    ShaderAssets shaders;
    const AssetId id = shaders.load(file.getPath());
    EXPECT_EQ(shaders.load(file.getPath()), id);
    EXPECT_EQ(shaders.getCount(), 1U);
    EXPECT_EQ(shaders.takeAdded().size(), 1U);
}

TEST(ShaderAssets, AddsAShaderWrittenInCode) {
    ShaderAssets shaders;
    const AssetId id = shaders.add("tint", kTint);
    EXPECT_EQ(id, AssetId::fromPath("tint"));
    ASSERT_NE(shaders.get(id), nullptr);
    EXPECT_EQ(shaders.get(id)->materialValues.front().name, "tint");
}

TEST(ShaderAssets, RecordsAShaderThatDoesNotCompile) {
    ShaderAssets shaders;
    const AssetId id = shaders.add("broken", kBroken);
    EXPECT_EQ(shaders.get(id), nullptr);
    ASSERT_NE(shaders.getEntry(id), nullptr);
    EXPECT_TRUE(shaders.getEntry(id)->error.contains("line 1"));
    EXPECT_TRUE(shaders.takeAdded().empty());
}

TEST(ShaderAssets, RecordsAMissingFile) {
    ShaderAssets shaders;
    const AssetId id = shaders.load(std::filesystem::temp_directory_path() / "rtype_missing_shader.glsl");
    EXPECT_EQ(shaders.get(id), nullptr);
    ASSERT_NE(shaders.getEntry(id), nullptr);
    EXPECT_FALSE(shaders.getEntry(id)->error.empty());
}

TEST(ShaderAssets, UnloadForgetsAShaderAndReportsIt) {
    ShaderAssets shaders;
    const AssetId id = shaders.add("tint", kTint);
    EXPECT_TRUE(shaders.unload(id));
    EXPECT_EQ(shaders.get(id), nullptr);
    EXPECT_EQ(shaders.takeUnloaded(), (std::vector<AssetId>{id}));
    EXPECT_FALSE(shaders.unload(id));
}

TEST(ShaderAssets, ReadsARelativePathFromItsDirectory) {
    const TemporaryFile file{"tint.glsl", kTint};
    ShaderAssets shaders;
    shaders.setDirectory(file.getPath().parent_path());
    const AssetId id = shaders.load("tint.glsl");
    EXPECT_EQ(id, AssetId::fromPath("tint.glsl"));
    EXPECT_NE(shaders.get(id), nullptr);
}

TEST(ShaderAssets, ReloadReplacesAShaderAndKeepsItsId) {
    const TemporaryFile file{"tint.glsl", kTint};
    ShaderAssets shaders;
    const AssetId id = shaders.load(file.getPath());
    static_cast<void>(shaders.takeAdded());
    std::ofstream{file.getPath(), std::ios::binary} << "uniform float other;\nvoid fragment() { COLOR.a *= other; }\n";
    EXPECT_TRUE(shaders.reload(id));
    EXPECT_TRUE(shaders.reload(id));
    ASSERT_NE(shaders.get(id), nullptr);
    EXPECT_EQ(shaders.get(id)->materialValues.front().name, "other");
    EXPECT_EQ(shaders.takeReloaded(), (std::vector<AssetId>{id}));
    EXPECT_TRUE(shaders.takeAdded().empty());
}

TEST(ShaderAssets, ReloadKeepsTheLastVersionWhenTheFileIsBroken) {
    const TemporaryFile file{"tint.glsl", kTint};
    ShaderAssets shaders;
    const AssetId id = shaders.load(file.getPath());
    std::ofstream{file.getPath(), std::ios::binary} << kBroken;
    EXPECT_FALSE(shaders.reload(id));
    ASSERT_NE(shaders.get(id), nullptr);
    EXPECT_EQ(shaders.get(id)->materialValues.front().name, "tint");
    EXPECT_TRUE(shaders.takeReloaded().empty());
}

TEST(ShaderAssets, ReloadGivesAFailedShaderItsFirstVersion) {
    const TemporaryFile file{"tint.glsl", kBroken};
    ShaderAssets shaders;
    const AssetId id = shaders.load(file.getPath());
    ASSERT_EQ(shaders.get(id), nullptr);
    std::ofstream{file.getPath(), std::ios::binary} << kTint;
    EXPECT_TRUE(shaders.reload(id));
    EXPECT_EQ(shaders.takeAdded(), (std::vector<AssetId>{id}));
    EXPECT_TRUE(shaders.takeReloaded().empty());
}

TEST(ShaderAssets, ReloadSkipsAShaderWrittenInCode) {
    ShaderAssets shaders;
    const AssetId id = shaders.add("tint", kTint);
    EXPECT_FALSE(shaders.reload(id));
    EXPECT_FALSE(shaders.reload(AssetId::fromPath("unknown")));
}
