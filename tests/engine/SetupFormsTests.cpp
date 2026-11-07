/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SetupFormsTests
*/

#include <gtest/gtest.h>

#include <format>
#include <memory>
#include <string>
#include <utility>

#include "core/asset/AssetId.hpp"
#include "core/library/LibraryEntry.hpp"
#include "core/system/ExecutablePath.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/engine/Engine.hpp"
#include "rtype/engine/plugin/PluginLoader.hpp"
#include "rtype/scripting/ScriptHost.hpp"

namespace {
/// @brief A server App with the plugin libraries loaded but no plugin added, as on a dedicated server: it has the
/// setup forms, which find none of their calls.
struct BareApp {
    rtype::engine::plugin::PluginLoader loader;
    rtype::ecs::App app{rtype::ecs::RunMode::kServer};
    std::unique_ptr<rtype::scripting::ScriptHost> host;

    BareApp() {
        loader.load(rtype::core::system::getExecutableDirectory() / rtype::engine::Engine::kPluginsDirectory);
        rtype::core::library::LibraryRegistrar registrar;
        registrar.provide(rtype::ecs::kPluginRegistryService, app.getPluginRegistry());
        loader.registerInto(registrar);
        auto created = rtype::scripting::ScriptHost::create(app, {.isServer = true});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("setup-forms-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }
};

/// @return How a script prints the id of the asset at @p path.
std::string printedId(const std::string& path) {
    return std::format("AssetId({:016x})", rtype::core::asset::AssetId::fromPath(path).value);
}
}  // namespace

TEST(SetupForms, WithoutTheirPluginsAssetFormsReturnTheIdTheAssetWouldHave) {
    const BareApp script;
    EXPECT_EQ(script.run(std::format(R"(
        assert(tostring(engine.assets.texture("assets/./ship.png", {{ linear = true, wrap = true }})) == "{}")
        assert(tostring(engine.assets.sound("assets/shot.ogg", {{ stream = false }})) == "{}")
        assert(tostring(engine.assets.font("assets/fonts/main.json")) == "{}")
        assert(tostring(engine.assets.tilemap("assets/level.tmj")) == "{}")
        assert(tostring(engine.assets.atlas({{ path = "assets/explosion.json" }})) == "{}")
        assert(tostring(engine.assets.atlas({{ texture = "assets/ship.png", name = "game.ship" }})) == "{}")
        assert(tostring(engine.assets.atlas({{ texture = "assets/laser.png" }})) == "{}")
    )",
                                     printedId("assets/ship.png"), printedId("assets/shot.ogg"),
                                     printedId("assets/fonts/main.json"), printedId("assets/level.tmj"),
                                     printedId("assets/explosion.json"), printedId("game.ship"),
                                     printedId("assets/laser.png"))),
              "");
}

TEST(SetupForms, WithoutTheInputPluginAnActionIsCheckedAndSkipped) {
    const BareApp script;
    EXPECT_EQ(script.run(R"(
        local result = engine.input.action("move", "vector2", {
            bind = { { up = "W", down = "S", left = "A", right = "D" }, { x = "GamepadLeftX", y = "GamepadLeftY" } },
        })
        assert(result == nil)
        engine.input.action("shoot")
    )"),
              "");
    EXPECT_NE(script.run(R"(engine.input.action("move", "vector2", { bind = { { up = "W" } } }))")
                  .find("engine.input.action: binding 1: expected a control name"),
              std::string::npos);
}

TEST(SetupForms, MistakesAreErrorsNamingTheForm) {
    const BareApp script;
    EXPECT_NE(script.run(R"(engine.assets.texture("ship.png", { filter = "linear" }))")
                  .find("engine.assets.texture: options: unexpected 'filter', expected 'linear', 'wrap'"),
              std::string::npos);
    EXPECT_NE(script.run(R"(engine.assets.texture("ship.png", { linear = 1 }))")
                  .find("engine.assets.texture: options: 'linear' must be a boolean"),
              std::string::npos);
    EXPECT_NE(script.run(R"(engine.assets.font())").find("engine.assets.font: expected the file's path first"),
              std::string::npos);
    EXPECT_NE(script.run(R"(engine.assets.tilemap("level.tmj", { stream = true }))")
                  .find("engine.assets.tilemap: options: unexpected 'stream': it takes no keys"),
              std::string::npos);
    EXPECT_NE(script.run(R"(engine.assets.atlas({ path = "a.json", texture = "a.png" }))")
                  .find("engine.assets.atlas: expected either a 'path'"),
              std::string::npos);
    EXPECT_NE(script.run(R"(engine.assets.atlas({ path = "a.json", grid = {} }))")
                  .find("engine.assets.atlas: 'name', 'grid' and 'frames' make an atlas from a 'texture'"),
              std::string::npos);
    EXPECT_NE(script.run(R"(engine.assets.atlas({ texture = "a.png", grid = { count = 2 } }))")
                  .find("engine.assets.atlas: 'grid': expected a 'frameSize'"),
              std::string::npos);
    EXPECT_NE(script.run(R"(engine.assets.atlas({ texture = "a.png", frames = { { position = { x = 0 } } } }))")
                  .find("engine.assets.atlas: frame 1: expected a 'position' and a 'size'"),
              std::string::npos);
    EXPECT_NE(script
                  .run(R"(engine.assets.atlas({
                  texture = "a.png", frames = { { position = { x = 0 }, size = { x = 1, y = 1 } } },
              }))")
                  .find("engine.assets.atlas: frame 1: 'position': missing 'y'"),
              std::string::npos);
    EXPECT_NE(script.run(R"(engine.assets.atlas({ texture = "a.png", animations = { fly = { frames = { 0, 2 } } } }))")
                  .find("engine.assets.atlas: animation 'fly': 'frames' must list frame indices that follow"),
              std::string::npos);
    EXPECT_NE(
        script
            .run(R"(engine.assets.atlas({ texture = "a.png", animations = { fly = { frames = { 0 }, first = 0 } } }))")
            .find("engine.assets.atlas: animation 'fly': expected either 'frames' or 'first' and 'count'"),
        std::string::npos);
    EXPECT_NE(script.run(R"(engine.assets.atlas({ texture = "a.png", frames = { [2] = {} } }))")
                  .find("engine.assets.atlas: 'frames': expected a list"),
              std::string::npos);
    EXPECT_NE(script.run(R"(engine.input.action("shoot", "button", { bind = { 3 } }))")
                  .find("engine.input.action: binding 1: expected a table"),
              std::string::npos);
    EXPECT_NE(script.run(R"(engine.input.action("shoot", "button", { keys = {} }))")
                  .find("engine.input.action: options: unexpected 'keys', expected 'bind'"),
              std::string::npos);
}

TEST(SetupForms, AParticleFormIsCheckedAndGivesTheIdWithoutThePlugin) {
    const BareApp script;
    EXPECT_EQ(script.run(std::format(R"(
        local sparks = engine.assets.particle({{
            name = "sparks", rate = 20, lifetime = {{ min = 0.2, max = 0.4 }},
            bursts = {{ {{ count = 8 }} }}, colorOverLife = {{ {{ t = 0, color = {{ 1, 1, 1 }} }} }},
        }})
        assert(tostring(sparks) == "{}")
    )",
                                     printedId("sparks"))),
              "");
    EXPECT_NE(script.run(R"(engine.assets.particle({ name = "a", lifetime = "long" }))")
                  .find("engine.assets.particle: 'lifetime': expected a number or a table"),
              std::string::npos);
}

TEST(SetupForms, ShaderFormsGiveTheIdsWithoutThePlugin) {
    const BareApp script;
    ASSERT_EQ(script.run(R"(
        local flash = engine.assets.shader("flash.glsl")
        engine.assets.material({ name = "enemy_hit", shader = flash, values = { strength = 0.5 } })
    )"),
              "");
}
