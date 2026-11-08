/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SettingsLoaderTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <vector>

#include "rtype/core/config/Settings.hpp"
#include "rtype/engine/CommandLine.hpp"
#include "rtype/engine/Exceptions.hpp"
#include "rtype/engine/scripting/SettingsLoader.hpp"
#include "rtype/luau/Exceptions.hpp"

namespace {

using rtype::engine::exceptions::RuntimeError;
using rtype::engine::scripting::SettingsLoader;

/// @brief Writes a temporary Luau script removed on destruction.
class TempScript {
  public:
    explicit TempScript(const std::string& content)
        : _path{std::filesystem::temp_directory_path() / std::format("rtype_settings_loader_{}.luau", ++_counter)} {
        std::ofstream{_path, std::ios::binary} << content;
    }

    ~TempScript() { std::filesystem::remove(_path); }
    TempScript(const TempScript&) = delete;
    TempScript& operator=(const TempScript&) = delete;
    TempScript(TempScript&&) = delete;
    TempScript& operator=(TempScript&&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const { return _path; }

  private:
    static inline int _counter = 0;  ///< Makes the file names unique.
    std::filesystem::path _path;     ///< Where the script is written.
};

/// Runs `source` and returns what it set.
rtype::core::config::Settings run(const std::string& source) {
    SettingsLoader loader;
    const TempScript script{source};

    loader.load(script.path());
    return loader.build();
}

}  // namespace

TEST(SettingsLoader, BuildsEmptySettingsByDefault) {
    SettingsLoader loader;

    EXPECT_FALSE(loader.build().has("anything"));
}

TEST(SettingsLoader, ExposesPureStandardLibraries) {
    const auto settings = run(R"(
        local plugins = {}
        table.insert(plugins, string.format("%s-%d", "a", math.floor(1.5)))
        engine.settings.plugins = plugins
        engine.settings.hasOs = os ~= nil
        engine.settings.hasDebug = debug ~= nil
    )");

    EXPECT_EQ(settings.getStringList("plugins", {}), (std::vector<std::string>{"a-1"}));
    EXPECT_FALSE(settings.getBool("hasOs", true));
    EXPECT_FALSE(settings.getBool("hasDebug", true));
}

TEST(SettingsLoader, StoresScalars) {
    const auto settings = run(R"(
        engine.settings.renderer = "vulkan"
        engine.settings.debugging = true
        engine.settings.window.width = 800
    )");

    EXPECT_EQ(settings.getString("renderer", ""), "vulkan");
    EXPECT_TRUE(settings.getBool("debugging", false));
    EXPECT_DOUBLE_EQ(settings.getNumber("window.width", 0), 800);
}

TEST(SettingsLoader, FlattensNestedTablesIntoDottedKeys) {
    const auto settings = run(R"(
        engine.settings.audio = { volume = { music = 0.5 } }
    )");

    EXPECT_DOUBLE_EQ(settings.getNumber("audio.volume.music", 0), 0.5);
    EXPECT_DOUBLE_EQ(settings.section("audio").getNumber("volume.music", 0), 0.5);
}

TEST(SettingsLoader, StoresNumberAndStringArrays) {
    const auto settings = run(R"(
        engine.settings.window.size = { 800, 600 }
        engine.settings.layers = { "a", "b" }
    )");

    EXPECT_EQ(settings.getNumberList("window.size", {}), (std::vector<double>{800, 600}));
    EXPECT_EQ(settings.getStringList("layers", {}), (std::vector<std::string>{"a", "b"}));
}

TEST(SettingsLoader, GroupsNeedNoDeclaration) {
    const auto settings = run(R"(
        engine.settings.window.title = "R-Type"
        engine.settings.physics.debug.color.red = 1
    )");

    EXPECT_EQ(settings.getString("window.title", ""), "R-Type");
    EXPECT_DOUBLE_EQ(settings.getNumber("physics.debug.color.red", 0), 1);
}

TEST(SettingsLoader, ReadOnlyGroupsStoreNothing) {
    const auto settings = run("local _ = engine.settings.window.title");

    EXPECT_FALSE(settings.has("window"));
    EXPECT_NO_THROW(settings.section("window").checkKeys({}, "window"));
}

TEST(SettingsLoader, FlagsAreVisibleToScripts) {
    SettingsLoader loader;
    const TempScript script{R"(
        if engine.isClient and not engine.isServer then
            engine.settings.side = "client"
        end
    )"};

    loader.setFlag("isClient", true);
    loader.setFlag("isServer", false);
    loader.load(script.path());

    EXPECT_EQ(loader.build().getString("side", ""), "client");
}

TEST(SettingsLoader, SetFlagReplacesThePreviousValue) {
    SettingsLoader loader;
    const TempScript script{R"(engine.settings.debug = engine.isDebug)"};

    loader.setFlag("isDebug", true);
    loader.setFlag("isDebug", false);
    loader.load(script.path());

    EXPECT_FALSE(loader.build().getBool("debug", true));
}

TEST(SettingsLoader, ArgsAreEmptyByDefault) {
    const rtype::core::config::Settings settings = run(R"(engine.settings.port = engine.args.port or 4242)");

    EXPECT_EQ(settings.getNumber("port", 0.0), 4242.0);
}

TEST(SettingsLoader, ArgsAreVisibleToScriptsWithTheirTypes) {
    SettingsLoader loader;
    const TempScript script{R"(
        engine.settings.port = engine.args.port
        engine.settings.host = engine.args.host
        engine.settings.verbose = engine.args.verbose
        engine.settings.fullscreen = engine.args.fullscreen
    )"};
    const std::vector<std::string> args{"--port", "4243", "--host", "10.0.0.2", "--verbose", "--fullscreen=false"};

    loader.setArgs(rtype::engine::CommandLine::parse(args));
    loader.load(script.path());

    const rtype::core::config::Settings settings = loader.build();
    EXPECT_EQ(settings.getNumber("port", 0.0), 4243.0);
    EXPECT_EQ(settings.getString("host", ""), "10.0.0.2");
    EXPECT_TRUE(settings.getBool("verbose", false));
    EXPECT_FALSE(settings.getBool("fullscreen", true));
}

TEST(SettingsLoader, SetArgsReplacesThePreviousArgs) {
    SettingsLoader loader;
    const TempScript script{R"(
        engine.settings.port = engine.args.port or 0
        engine.settings.host = engine.args.host or "none"
    )"};
    const std::vector<std::string> first{"--port", "1"};
    const std::vector<std::string> second{"--host", "localhost"};

    loader.setArgs(rtype::engine::CommandLine::parse(first));
    loader.setArgs(rtype::engine::CommandLine::parse(second));
    loader.load(script.path());

    const rtype::core::config::Settings settings = loader.build();
    EXPECT_EQ(settings.getNumber("port", -1.0), 0.0);
    EXPECT_EQ(settings.getString("host", ""), "localhost");
}

TEST(SettingsLoader, AccumulatesSettingsOverSeveralScripts) {
    SettingsLoader loader;
    const TempScript first{R"(engine.settings.a = 1)"};
    const TempScript second{R"(engine.settings.b = 2)"};

    loader.load(first.path());
    loader.load(second.path());
    const auto settings = loader.build();

    EXPECT_DOUBLE_EQ(settings.getNumber("a", 0), 1);
    EXPECT_DOUBLE_EQ(settings.getNumber("b", 0), 2);
}

TEST(SettingsLoader, RejectsAnUnsupportedValueType) {
    EXPECT_THROW(run("engine.settings.callback = function() end"), RuntimeError);
}

TEST(SettingsLoader, RejectsAnArrayOfMixedTypes) {
    EXPECT_THROW(run(R"(engine.settings.list = { 1, "two" })"), RuntimeError);
}

TEST(SettingsLoader, RejectsAnArrayOfUnsupportedElements) {
    EXPECT_THROW(run("engine.settings.list = { true, false }"), RuntimeError);
}

TEST(SettingsLoader, RejectsAnArrayMixedWithNamedKeys) {
    EXPECT_THROW(run("engine.settings.list = { 1, 2, name = 3 }"), RuntimeError);
}

TEST(SettingsLoader, RejectsANonStringKey) { EXPECT_THROW(run("engine.settings.group = { [10] = 1 }"), RuntimeError); }

TEST(SettingsLoader, RejectsReadingAGroupByANonStringKey) {
    EXPECT_THROW(run("local _ = engine.settings[1]"), rtype::luau::exceptions::RuntimeError);
}

TEST(SettingsLoader, NamesTheOffendingSettingInTheError) {
    try {
        run("engine.settings.window.title = function() end");
        FAIL() << "expected a RuntimeError";
    } catch (const RuntimeError& error) {
        EXPECT_NE(std::string{error.what()}.find("window.title"), std::string::npos);
    }
}

TEST(SettingsLoader, LoadThrowsOnAMissingFile) {
    SettingsLoader loader;

    EXPECT_THROW(loader.load("does/not/exist.luau"), rtype::luau::exceptions::RuntimeError);
}

TEST(SettingsLoader, LoadThrowsOnAScriptError) {
    SettingsLoader loader;
    const TempScript script{R"(error("boom"))"};

    EXPECT_THROW(loader.load(script.path()), rtype::luau::exceptions::RuntimeError);
}

TEST(SettingsLoader, ScriptsHaveNoOsLibrary) {
    SettingsLoader loader;
    const TempScript script{R"(engine.settings.x = os.time())"};

    EXPECT_THROW(loader.load(script.path()), rtype::luau::exceptions::RuntimeError);
}
