/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptingPluginTests
*/

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/scripting/exceptions/ScriptLoadException.hpp"
#include "rtype/scripting/plugin/ScriptingPlugin.hpp"

namespace {
/// @brief A scripts directory in the temporary directory, removed with it.
class ScriptsDirectory {
  public:
    explicit ScriptsDirectory(const std::string_view name)
        : _path{std::filesystem::temp_directory_path() / std::string{name}} {
        std::filesystem::remove_all(_path);
        std::filesystem::create_directories(_path);
    }

    ~ScriptsDirectory() {
        std::error_code error;
        std::filesystem::remove_all(_path, error);
    }

    ScriptsDirectory(const ScriptsDirectory& other) = delete;
    ScriptsDirectory& operator=(const ScriptsDirectory& other) = delete;
    ScriptsDirectory(ScriptsDirectory&& other) = delete;
    ScriptsDirectory& operator=(ScriptsDirectory&& other) = delete;

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return _path; }

    /// @brief Writes a file under the directory, creating its folders.
    void write(const std::string& relative, const std::string_view content) const {
        const auto file = _path / relative;
        std::filesystem::create_directories(file.parent_path());
        std::ofstream{file} << content;
    }

  private:
    std::filesystem::path _path;  ///< The directory.
};

/// @brief Adds the plugin to an App that runs one frame per second.
void addPlugin(rtype::ecs::App& app, const std::filesystem::path& directory,
               const rtype::scripting::ScriptHost::Options options = {}) {
    app.addPlugin(rtype::ecs::TimePlugin::manual(std::chrono::seconds{1}, rtype::ecs::FixedTimestep{1, 5}));
    app.addPlugin(rtype::scripting::ScriptingPlugin{{.directory = directory, .options = options}});
}
}  // namespace

TEST(ScriptingPlugin, LoadsTheFilesOfTheDirectoryAndOfItsSubfolders) {
    const ScriptsDirectory scripts{"rtype-scripting-plugin-recursive"};
    scripts.write("a.luau", R"(engine.ecs.component("game.A", { fields = {} }))");
    scripts.write("nested/deep/b.luau", R"(engine.ecs.component("game.B", { fields = {} }))");
    scripts.write("notes.txt", "this is not a script and is not a Luau chunk");

    rtype::ecs::App app;
    addPlugin(app, scripts.path());

    EXPECT_TRUE(app.hasPlugin("rtype.scripting"));
    EXPECT_TRUE(app.getWorld().getComponents().find("game.A"));
    EXPECT_TRUE(app.getWorld().getComponents().find("game.B"));
}

TEST(ScriptingPlugin, LoadsInSortedPathOrder) {
    const ScriptsDirectory scripts{"rtype-scripting-plugin-order"};
    // Written in the reverse order: the second file only works if the first one ran before.
    scripts.write("b_uses.luau", R"(assert(order == "a", "a.luau did not run first"); order = "ab")");
    scripts.write("a_first.luau", R"(order = "a")");
    scripts.write("c_checks.luau", R"(assert(order == "ab"); engine.ecs.component("game.Done", { fields = {} }))");

    rtype::ecs::App app;
    addPlugin(app, scripts.path());

    EXPECT_TRUE(app.getWorld().getComponents().find("game.Done"));
}

TEST(ScriptingPlugin, ScriptsSeeTheFlagsOfTheConfig) {
    const ScriptsDirectory scripts{"rtype-scripting-plugin-flags"};
    scripts.write("flags.luau", R"(
        if engine.isServer and not engine.isClient then
            engine.ecs.component("game.ServerOnly", { fields = {} })
        end
    )");

    rtype::ecs::App app;
    addPlugin(app, scripts.path(), {.isClient = false, .isServer = true});

    EXPECT_TRUE(app.getWorld().getComponents().find("game.ServerOnly"));
}

TEST(ScriptingPlugin, ASystemLoadedFromAFileRuns) {
    const ScriptsDirectory scripts{"rtype-scripting-plugin-system"};
    scripts.write("shield.luau", R"(
        engine.ecs.component("game.Shield", { fields = { strength = "f32" }, defaults = { strength = 10 } })
        engine.ecs.system("game.Drain", {
            phase = "FixedUpdate",
            query = { write = { "game.Shield" } },
            run = function(ctx)
                for entity, shield in ctx.query do
                    shield.strength = shield.strength - 1
                end
            end,
        })
    )");

    rtype::ecs::App app;
    addPlugin(app, scripts.path());

    auto& components = app.getWorld().getComponents();
    const auto id = components.find("game.Shield");
    ASSERT_TRUE(id);
    const auto ship = app.getWorld().spawn();
    const float strength = 10.F;
    std::array<std::byte, sizeof(float)> bytes{};
    std::memcpy(bytes.data(), &strength, sizeof(float));
    ASSERT_TRUE(components.add(ship, *id, bytes));

    app.runFrame();

    float drained = 0.F;
    std::memcpy(&drained, components.get(ship, *id).data(), sizeof(float));
    EXPECT_FLOAT_EQ(drained, 9.F);
}

TEST(ScriptingPlugin, AMissingDirectoryLoadsNothingAndWarns) {
    rtype::ecs::App app;
    testing::internal::CaptureStderr();
    EXPECT_NO_THROW(addPlugin(app, std::filesystem::temp_directory_path() / "rtype-scripting-plugin-missing"));
    const std::string warning = testing::internal::GetCapturedStderr();

    EXPECT_TRUE(app.hasPlugin("rtype.scripting"));
    EXPECT_NE(warning.find("rtype-scripting-plugin-missing"), std::string::npos) << warning;
}

TEST(ScriptingPlugin, AFailingScriptIsAnErrorNamingTheFile) {
    const ScriptsDirectory scripts{"rtype-scripting-plugin-failing"};
    scripts.write("broken.luau", R"(error("boom"))");

    rtype::ecs::App app;
    try {
        addPlugin(app, scripts.path());
        FAIL() << "the plugin should have thrown";
    } catch (const rtype::scripting::exceptions::ScriptLoadException& e) {
        EXPECT_NE(std::string{e.what()}.find("broken.luau"), std::string::npos) << e.what();
        EXPECT_NE(std::string{e.what()}.find("boom"), std::string::npos) << e.what();
    }
}
