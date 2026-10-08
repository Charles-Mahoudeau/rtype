/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RuntimeTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>

#include "rtype/luau/ErrorKind.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/RuntimeConfig.hpp"

namespace {
constexpr rtype::luau::RuntimeConfig kStandardConfig{
    .libs = rtype::luau::RuntimeConfig::Libs::kStandard,
    .optimizationLevel = rtype::luau::RuntimeConfig::OptimizationLevel::kStandard,
};

rtype::luau::Runtime makeRuntime(const rtype::luau::RuntimeConfig config = kStandardConfig) {
    auto runtime = rtype::luau::Runtime::create(config);
    EXPECT_TRUE(runtime);
    return std::move(*runtime);
}

/// @brief Writes a temporary file removed on destruction.
class TempFile {
  public:
    TempFile(const std::string& name, const std::string& content)
        : _path{std::filesystem::temp_directory_path() / name} {
        std::ofstream{_path, std::ios::binary} << content;
    }

    ~TempFile() { std::filesystem::remove(_path); }
    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
    TempFile(TempFile&&) = delete;
    TempFile& operator=(TempFile&&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const { return _path; }

  private:
    std::filesystem::path _path;
};
}  // namespace

TEST(Runtime, CreateReturnsRuntime) {
    const auto runtime = rtype::luau::Runtime::create(kStandardConfig);

    EXPECT_TRUE(runtime);
}

TEST(Runtime, CreateWithNoLibrariesSucceeds) {
    const auto runtime = rtype::luau::Runtime::create({
        .libs = rtype::luau::RuntimeConfig::Libs::kNone,
        .optimizationLevel = rtype::luau::RuntimeConfig::OptimizationLevel::kDisabled,
    });

    EXPECT_TRUE(runtime);
}

TEST(Runtime, CreateWithPartialLibrariesFails) {
    const auto runtime = rtype::luau::Runtime::create({
        .libs = rtype::luau::RuntimeConfig::Libs::kBase | rtype::luau::RuntimeConfig::Libs::kMath,
        .optimizationLevel = rtype::luau::RuntimeConfig::OptimizationLevel::kStandard,
    });

    ASSERT_FALSE(runtime);
    EXPECT_EQ(runtime.error().kind(), rtype::luau::ErrorKind::kUnknown);
}

TEST(Runtime, IsMovable) {
    auto runtime = makeRuntime();

    rtype::luau::Runtime moved{std::move(runtime)};
    auto other = makeRuntime();
    other = std::move(moved);

    const auto script = other.load("moved", "local x = 1");
    EXPECT_TRUE(script);
}

TEST(Runtime, LoadValidSourceReturnsScript) {
    const auto runtime = makeRuntime();

    const auto script = runtime.load("valid", "local x = 1 + 1");

    ASSERT_TRUE(script);
    EXPECT_EQ(script->name(), "valid");
    EXPECT_NE(script->state(), nullptr);
}

TEST(Runtime, LoadSyntaxErrorReturnsCompilationError) {
    const auto runtime = makeRuntime();

    const auto script = runtime.load("broken", "local = = 1");

    ASSERT_FALSE(script);
    EXPECT_EQ(script.error().kind(), rtype::luau::ErrorKind::kCompilation);
}

TEST(Runtime, LoadDoesNotRunTheScript) {
    const auto runtime = makeRuntime();

    const auto script = runtime.load("lazy", "error('should not run')");

    EXPECT_TRUE(script);
}

TEST(Runtime, LoadFromFileNamesScriptAfterFilename) {
    const auto runtime = makeRuntime();
    const TempFile file{"rtype_runtime_tests_load.luau", "local x = 1"};

    const auto script = runtime.load(file.path());

    ASSERT_TRUE(script);
    EXPECT_EQ(script->name(), "rtype_runtime_tests_load.luau");
}

TEST(Runtime, LoadMissingFileReturnsError) {
    const auto runtime = makeRuntime();

    const auto script = runtime.load(std::filesystem::temp_directory_path() / "rtype_runtime_tests_missing.luau");

    ASSERT_FALSE(script);
    EXPECT_EQ(script.error().kind(), rtype::luau::ErrorKind::kUnknown);
}

TEST(Runtime, LoadFileWithSyntaxErrorReturnsCompilationError) {
    const auto runtime = makeRuntime();
    const TempFile file{"rtype_runtime_tests_broken.luau", "local = = 1"};

    const auto script = runtime.load(file.path());

    ASSERT_FALSE(script);
    EXPECT_EQ(script.error().kind(), rtype::luau::ErrorKind::kCompilation);
}

TEST(Runtime, RunValidScriptSucceeds) {
    const auto runtime = makeRuntime();
    const auto script = runtime.load("ok", "local x = 1 + 1");
    ASSERT_TRUE(script);

    EXPECT_TRUE(runtime.run(*script));
}

TEST(Runtime, RunClosureSucceeds) {
    const auto runtime = makeRuntime();
    const auto script = runtime.load("ok", "local x = 1");
    ASSERT_TRUE(script);

    EXPECT_TRUE(runtime.run(script->closure()));
}

TEST(Runtime, RunTwiceSucceeds) {
    const auto runtime = makeRuntime();
    const auto script = runtime.load("twice", "local x = 1");
    ASSERT_TRUE(script);

    EXPECT_TRUE(runtime.run(*script));
    EXPECT_TRUE(runtime.run(*script));
}

TEST(Runtime, RunRaisingScriptReturnsRuntimeError) {
    const auto runtime = makeRuntime();
    const auto script = runtime.load("raise", "error('boom')");
    ASSERT_TRUE(script);

    const auto result = runtime.run(*script);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kRuntime);
    EXPECT_NE(result.error().message().find("boom"), std::string_view::npos);
}

TEST(Runtime, RunAfterFailureStillWorks) {
    const auto runtime = makeRuntime();
    const auto failing = runtime.load("raise", "error('boom')");
    const auto working = runtime.load("ok", "local x = 1");
    ASSERT_TRUE(failing);
    ASSERT_TRUE(working);

    EXPECT_FALSE(runtime.run(*failing));
    EXPECT_TRUE(runtime.run(*working));
}

TEST(Runtime, StandardLibrariesAreAvailable) {
    const auto runtime = makeRuntime();
    const auto script = runtime.load("libs", "assert(math.max(1, 2) == 2 and string.len('ab') == 2)");
    ASSERT_TRUE(script);

    EXPECT_TRUE(runtime.run(*script));
}

TEST(Runtime, NoLibrariesLeavesGlobalsEmpty) {
    const auto runtime = makeRuntime({
        .libs = rtype::luau::RuntimeConfig::Libs::kNone,
        .optimizationLevel = rtype::luau::RuntimeConfig::OptimizationLevel::kStandard,
    });
    const auto script = runtime.load("nolibs", "local m = math\nlocal _ = m.max");
    ASSERT_TRUE(script);

    const auto result = runtime.run(*script);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kRuntime);
}

TEST(Runtime, ScriptRunsInItsOwnThread) {
    const auto runtime = makeRuntime();
    const auto first = runtime.load("first", "local x = 1");
    const auto second = runtime.load("second", "local x = 2");
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);

    EXPECT_NE(first->state(), second->state());
}

TEST(Runtime, ScriptGlobalsDoNotLeakToOtherScripts) {
    const auto runtime = makeRuntime();
    const auto writer = runtime.load("writer", "shared = 42");
    const auto reader = runtime.load("reader", "assert(shared == nil)");
    ASSERT_TRUE(writer);
    ASSERT_TRUE(reader);

    EXPECT_TRUE(runtime.run(*writer));
    EXPECT_TRUE(runtime.run(*reader));
}

TEST(Runtime, ScriptKeepsItsOwnGlobalsBetweenRuns) {
    const auto runtime = makeRuntime();
    const auto script = runtime.load("counter", "count = (count or 0) + 1\nassert(count <= 2)");
    ASSERT_TRUE(script);

    EXPECT_TRUE(runtime.run(*script));
    EXPECT_TRUE(runtime.run(*script));
}

TEST(Runtime, ScriptCannotModifyStandardLibraries) {
    const auto runtime = makeRuntime();
    const auto script = runtime.load("tamper", "math.max = nil");
    ASSERT_TRUE(script);

    const auto result = runtime.run(*script);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kRuntime);
}

TEST(Runtime, ScriptSurvivesRuntimeGarbageCollection) {
    const auto runtime = makeRuntime();
    const auto script = runtime.load("gc", "assert(1 + 1 == 2)");
    ASSERT_TRUE(script);

    lua_gc(script->state(), LUA_GCCOLLECT, 0);

    EXPECT_TRUE(runtime.run(*script));
}

TEST(Runtime, AllOptimizationLevelsRunScripts) {
    for (const auto level : {rtype::luau::RuntimeConfig::OptimizationLevel::kDisabled,
                             rtype::luau::RuntimeConfig::OptimizationLevel::kStandard,
                             rtype::luau::RuntimeConfig::OptimizationLevel::kAggressive}) {
        const auto runtime =
            makeRuntime({.libs = rtype::luau::RuntimeConfig::Libs::kStandard, .optimizationLevel = level});
        const auto script = runtime.load("opt", "local function f(a) return a * 2 end assert(f(2) == 4)");
        ASSERT_TRUE(script);

        EXPECT_TRUE(runtime.run(*script));
    }
}
