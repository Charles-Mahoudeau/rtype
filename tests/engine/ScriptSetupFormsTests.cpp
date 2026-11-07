/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptSetupFormsTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <variant>

#include "core/asset/AssetId.hpp"
#include "core/config/Settings.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/input/Input.hpp"
#include "plugins/input/InputAction.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/input/PlatformEvent.hpp"
#include "plugins/particle/ParticleEffect.hpp"
#include "plugins/particle/ParticleEffectAssets.hpp"
#include "plugins/particle/ParticlePlugin.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/ShaderAssets.hpp"
#include "plugins/shader/ShaderPlugin.hpp"
#include "plugins/sprite/AtlasAssets.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"
#include "plugins/sprite/SpritePlugin.hpp"
#include "plugins/text/FontAssets.hpp"
#include "plugins/text/TextPlugin.hpp"
#include "plugins/tilemap/TilemapAssets.hpp"
#include "plugins/tilemap/TilemapPlugin.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/contract/platform/event/Event.hpp"
#include "rtype/contract/platform/input/Key.hpp"
#include "rtype/contract/renderer/drawing/BlendMode.hpp"
#include "rtype/contract/renderer/particle/Particle.hpp"
#include "rtype/contract/renderer/particle/ParticleSystemDesc.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/scripting/ScriptHost.hpp"

namespace {
/// @brief A standalone App and a script host on it; each test adds the plugins its forms use.
struct ScriptApp {
    rtype::ecs::App app;
    std::unique_ptr<rtype::scripting::ScriptHost> host;

    ScriptApp() {
        auto created = rtype::scripting::ScriptHost::create(app, {.isClient = true, .isServer = true});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("setup-forms-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }

    /// @brief Sends a key press to the InputPlugin, as the backend plugin does after polling the window.
    void press(const rtype::contract::platform::input::Key key) {
        app.getWorld().getEvents().send(
            rtype::plugins::input::PlatformEvent{.event = rtype::contract::platform::event::KeyPressed{.key = key}});
    }
};

/// @return The atlas @p name names in @p app, or nullptr.
const rtype::plugins::sprite::SpriteAtlas* atlasOf(rtype::ecs::App& app, const std::string& name) {
    return app.getWorld().getResources().require<rtype::plugins::sprite::AtlasAssets>().get(
        rtype::core::asset::AssetId::fromPath(name));
}
}  // namespace

TEST(ScriptSetupForms, AnAtlasFormCutsFramesAndNamesAnimations) {
    ScriptApp script;
    script.app.addPlugin(rtype::plugins::sprite::SpritePlugin{});
    ASSERT_EQ(script.run(R"(
        local ship = engine.assets.atlas({
            name = "game.ship", texture = "ship.png",
            grid = { frameSize = { x = 48, y = 48 }, count = 5, spacing = { x = 17, y = 0 } },
            animations = {
                fly = { frames = { 0, 1, 2, 3, 4 }, frameDuration = 0.08, direction = "pingPong" },
                bank = { first = 3, plays = 1 },
            },
        })

        local laser = engine.assets.atlas({
            texture = "laser.png", linear = true,
            frames = {
                { position = { x = 108, y = 151 }, size = { x = 80, y = 16 } },
                { name = "end", position = { x = 189, y = 151 }, size = { x = 80, y = 16 }, origin = { x = 0, y = 8 } },
            },
            animations = { glow = { frames = { 0, 1 } } },
        })
        assert(laser ~= ship)
    )"),
              "");

    const auto* ship = atlasOf(script.app, "game.ship");
    ASSERT_NE(ship, nullptr);
    EXPECT_EQ(ship->getFrames().size(), 5U);
    const auto* fly = ship->findAnimation(rtype::plugins::sprite::animationId("fly"));
    ASSERT_NE(fly, nullptr);
    EXPECT_EQ(fly->frames.size(), 5U);
    EXPECT_FLOAT_EQ(fly->durations.front(), 0.08F);
    EXPECT_EQ(fly->direction, rtype::plugins::sprite::AnimationDirection::kPingPong);
    const auto* bank = ship->findAnimation(rtype::plugins::sprite::animationId("bank"));
    ASSERT_NE(bank, nullptr);
    EXPECT_EQ(bank->frames.size(), 2U);
    EXPECT_EQ(bank->repeat, 1U);

    const auto* laser = atlasOf(script.app, "laser.png");
    ASSERT_NE(laser, nullptr);
    ASSERT_EQ(laser->getFrames().size(), 2U);
    EXPECT_FLOAT_EQ(laser->getFrames().at(1).source.getX(), 189.0F);
    EXPECT_FLOAT_EQ(laser->getFrames().at(1).origin.y, 8.0F);
    EXPECT_NE(laser->findAnimation(rtype::plugins::sprite::animationId("glow")), nullptr);
}

TEST(ScriptSetupForms, AnAtlasFormReportsTheSpritePluginsErrors) {
    ScriptApp script;
    script.app.addPlugin(rtype::plugins::sprite::SpritePlugin{});
    const std::string error = script.run(R"(
        engine.assets.atlas({ texture = "ship.png", animations = { fly = { direction = "sideways" } } })
    )");
    EXPECT_NE(error.find("engine.assets.atlas: engine call 'rtype.sprite.addAnimation': Unknown animation direction"),
              std::string::npos)
        << error;
}

TEST(ScriptSetupForms, AnInputFormDeclaresAnActionAndItsBindings) {
    ScriptApp script;
    script.app.addPlugin(rtype::plugins::input::InputPlugin{});
    ASSERT_EQ(script.run(R"(
        engine.input.action("move", "vector2", {
            bind = { { up = "W", down = "S", left = "A", right = "D" }, { x = "GamepadLeftX", y = "GamepadLeftY" } },
        })
        engine.input.action("steer", "axis", { bind = { { negative = "Left", positive = "Right" } } })
        engine.input.action("boost", "axis", { bind = { { control = "B", scale = -2 } } })
        engine.input.action("shoot", nil, { bind = { "Space", "MouseLeft" } })
    )"),
              "");

    script.press(rtype::contract::platform::input::Key::kD);
    script.press(rtype::contract::platform::input::Key::kLeft);
    script.press(rtype::contract::platform::input::Key::kB);
    script.press(rtype::contract::platform::input::Key::kSpace);
    script.app.runFrame();

    const auto& input = script.app.getWorld().getResources().require<rtype::plugins::input::Input>();
    EXPECT_FLOAT_EQ(input.getAction("move").readVector().x, 1.0F);
    EXPECT_FLOAT_EQ(input.getAction("steer").readAxis(), -1.0F);
    EXPECT_FLOAT_EQ(input.getAction("boost").readAxis(), -2.0F);
    EXPECT_EQ(input.getAction("shoot").getType(), rtype::plugins::input::ActionType::kButton);
    EXPECT_TRUE(input.getAction("shoot").isPressed());
}

TEST(ScriptSetupForms, AnInputFormReportsUnknownControls) {
    ScriptApp script;
    script.app.addPlugin(rtype::plugins::input::InputPlugin{});
    const std::string error = script.run(R"(engine.input.action("jump", "button", { bind = { "Jump" } }))");
    EXPECT_NE(error.find("engine.input.action: engine call 'rtype.input.bind': Unknown control 'Jump'"),
              std::string::npos)
        << error;
}

TEST(ScriptSetupForms, FontAndTilemapFormsAskTheirPlugins) {
    ScriptApp script;
    script.app.addPlugin(rtype::plugins::transform::TransformPlugin{});
    script.app.addPlugin(rtype::plugins::input::InputPlugin{});
    script.app.addPlugin(rtype::plugins::graphics::GraphicsPlugin{
        std::make_unique<rtype::plugins::graphics::BackendRegistry>(), rtype::core::config::Settings{}});
    script.app.addPlugin(rtype::plugins::shader::ShaderPlugin{});
    script.app.addPlugin(rtype::plugins::text::TextPlugin{});
    script.app.addPlugin(rtype::plugins::tilemap::TilemapPlugin{});
    ASSERT_EQ(script.run(R"(
        engine.assets.font("missing-font.json")
        engine.assets.tilemap("missing.tmj")
    )"),
              "");

    const auto& resources = script.app.getWorld().getResources();
    EXPECT_NE(resources.require<rtype::plugins::text::FontAssets>().getEntry(
                  rtype::core::asset::AssetId::fromPath("missing-font.json")),
              nullptr);
    EXPECT_NE(resources.require<rtype::plugins::tilemap::TilemapAssets>().getEntry(
                  rtype::core::asset::AssetId::fromPath("missing.tmj")),
              nullptr);
}

namespace {
/// @brief Adds the plugins the shader forms use to @p script.
void addShaderPlugins(ScriptApp& script) {
    script.app.addPlugin(rtype::plugins::transform::TransformPlugin{});
    script.app.addPlugin(rtype::plugins::input::InputPlugin{});
    script.app.addPlugin(rtype::plugins::graphics::GraphicsPlugin{
        std::make_unique<rtype::plugins::graphics::BackendRegistry>(), rtype::core::config::Settings{}});
    script.app.addPlugin(rtype::plugins::shader::ShaderPlugin{});
}
}  // namespace

TEST(ScriptSetupForms, ShaderAndMaterialFormsFillTheirAssets) {
    ScriptApp script;
    addShaderPlugins(script);
    ASSERT_EQ(script.run(R"(
        local flash = engine.assets.shader("missing.glsl")
        local noise = engine.assets.texture("noise.png")
        engine.assets.material({
            name = "enemy_hit",
            shader = flash,
            values = { strength = 0.5, tint = { x = 1, y = 0.5, z = 0, w = 1 }, offset = { x = 2, y = 3 } },
            textures = { noise = noise },
        })
    )"),
              "");

    const auto& resources = script.app.getWorld().getResources();
    const rtype::core::asset::AssetId shader = rtype::core::asset::AssetId::fromPath("missing.glsl");
    EXPECT_NE(resources.require<rtype::plugins::shader::ShaderAssets>().getEntry(shader), nullptr);
    const rtype::plugins::shader::Material* material = resources.require<rtype::plugins::shader::MaterialAssets>().get(
        rtype::core::asset::AssetId::fromPath("enemy_hit"));
    ASSERT_NE(material, nullptr);
    EXPECT_EQ(material->shader, shader);
    EXPECT_EQ(material->values.size(), 3U);
    ASSERT_EQ(material->textures.size(), 1U);
    EXPECT_EQ(material->textures[0].texture, rtype::core::asset::AssetId::fromPath("noise.png"));
}

TEST(ScriptSetupForms, AMaterialFormGivesAnIntUniformAWholeNumber) {
    const std::filesystem::path file = std::filesystem::temp_directory_path() / "rtype-setup-forms-frames.glsl";
    std::ofstream{file, std::ios::binary} << "uniform int frames;\nvoid fragment() { COLOR.a *= float(frames); }\n";
    ScriptApp script;
    addShaderPlugins(script);
    ASSERT_EQ(script.run(std::format(R"(
        engine.assets.material({{
            name = "animated", shader = engine.assets.shader("{}"), values = {{ frames = 4 }},
        }})
    )",
                                     file.generic_string())),
              "");
    std::filesystem::remove(file);

    const rtype::plugins::shader::Material* material =
        script.app.getWorld().getResources().require<rtype::plugins::shader::MaterialAssets>().get(
            rtype::core::asset::AssetId::fromPath("animated"));
    ASSERT_NE(material, nullptr);
    ASSERT_EQ(material->values.size(), 1U);
    EXPECT_EQ(std::get<std::int32_t>(material->values[0].value), 4);
}

TEST(ScriptSetupForms, AMaterialFormChecksItsTable) {
    ScriptApp script;
    addShaderPlugins(script);
    EXPECT_NE(script.run(R"(engine.assets.material({ name = "a" }))").find("expected a 'name' and a 'shader'"),
              std::string::npos);
    const std::string error = script.run(R"(
        engine.assets.material({ name = "a", shader = engine.assets.shader("a.glsl"), values = { v = { x = 1 } } })
    )");
    EXPECT_NE(error.find("engine.assets.material: value 'v'"), std::string::npos) << error;
}

TEST(ScriptSetupForms, AParticleFormBuildsTheWholeEffect) {
    ScriptApp script;
    addShaderPlugins(script);
    script.app.addPlugin(rtype::plugins::particle::ParticlePlugin{});
    ASSERT_EQ(script.run(R"(
        local smoke = engine.assets.particle({
            name = "game.smoke",
            texture = engine.assets.texture("smoke.png"),
            source = { position = { x = 0, y = 0 }, size = { x = 64, y = 32 } },
            flipbook = { columns = 2, rows = 1 },
            blend = "alpha", simulation = "cpu", capacity = 128,
            rate = 30, shape = { type = "rect", size = { x = 16, y = 4 } },
            lifetime = { min = 0.5, max = 1 }, speed = 40, angle = { min = -0.3, max = 0.3 },
            color = { 0.5, 0.5, 0.5 }, gravity = { x = 0, y = -20 }, drag = 1.5,
            bursts = { { count = 10 }, { time = 0.5, count = 4, cycles = 2, interval = 0.1 } },
            colorOverLife = { { t = 0, color = { 1, 1, 1, 1 } }, { t = 1, color = { 1, 1, 1, 0 } } },
            sizeOverLife = { { t = 0, value = 0.5 }, { t = 1, value = 2 } },
        })
        assert(smoke == engine.assetId("game.smoke"))
    )"),
              "");

    const rtype::plugins::particle::ParticleEffect* smoke =
        script.app.getWorld().getResources().require<rtype::plugins::particle::ParticleEffectAssets>().get(
            rtype::core::asset::AssetId::fromPath("game.smoke"));
    ASSERT_NE(smoke, nullptr);
    EXPECT_EQ(smoke->texture, rtype::core::asset::AssetId::fromPath("smoke.png"));
    EXPECT_FLOAT_EQ(smoke->source.getWidth(), 64.0F);
    EXPECT_EQ(smoke->flipbook.columns, 2U);
    EXPECT_EQ(smoke->blend, rtype::contract::renderer::BlendMode::kAlpha);
    EXPECT_EQ(smoke->desc.simulation, rtype::contract::renderer::ParticleSimulation::kCpu);
    EXPECT_EQ(smoke->desc.capacity, 128U);
    EXPECT_FLOAT_EQ(smoke->desc.rate, 30.0F);
    const auto* rect = std::get_if<rtype::contract::renderer::RectShape>(&smoke->desc.shape);
    ASSERT_NE(rect, nullptr);
    EXPECT_FLOAT_EQ(rect->size.x, 16.0F);
    EXPECT_FLOAT_EQ(smoke->desc.speed.min, 40.0F);
    EXPECT_FLOAT_EQ(smoke->desc.speed.max, 40.0F);
    EXPECT_FLOAT_EQ(smoke->desc.angle.min, -0.3F);
    EXPECT_FLOAT_EQ(smoke->desc.color.w, 1.0F);  // the alpha left out is 1
    EXPECT_FLOAT_EQ(smoke->desc.gravity.y, -20.0F);
    ASSERT_EQ(smoke->desc.bursts.size(), 2U);
    EXPECT_EQ(smoke->desc.bursts[1].cycles, 2U);
    EXPECT_EQ(smoke->desc.colorOverLife.keys.size(), 2U);
    ASSERT_EQ(smoke->desc.sizeOverLife.keys.size(), 2U);
    EXPECT_FLOAT_EQ(smoke->desc.sizeOverLife.keys[1].value, 2.0F);
}

TEST(ScriptSetupForms, AParticleFormReportsItsMistakes) {
    ScriptApp script;
    addShaderPlugins(script);
    script.app.addPlugin(rtype::plugins::particle::ParticlePlugin{});
    const auto expectError = [&script](const std::string& source, const std::string& message) {
        const std::string error = script.run(source);
        EXPECT_NE(error.find(message), std::string::npos) << "got: " << error;
    };
    expectError(R"(engine.assets.particle({ rate = 1 }))", "engine.assets.particle: expected a 'name'");
    expectError(R"(engine.assets.particle({ name = "a", colour = 1 }))", "unexpected 'colour'");
    expectError(R"(engine.assets.particle({ name = "a", color = { 1, 1 } }))", "'color': expected a colour");
    expectError(R"(engine.assets.particle({ name = "a", bursts = { { time = 1 } } }))", "burst 1: expected a 'count'");
    expectError(R"(engine.assets.particle({ name = "a", sizeOverLife = { { value = 1 } } }))",
                "'sizeOverLife' key 1: expected a 't'");
    expectError(R"(engine.assets.particle({ name = "a", blend = "glow" }))",
                "Unknown blend 'glow' (expected 'additive', 'alpha', 'cutout')");
    expectError(R"(engine.assets.particle({ name = "a", lifetime = 0 }))", "Particle effect 'a'");
}
