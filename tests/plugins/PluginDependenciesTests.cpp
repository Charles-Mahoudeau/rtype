/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginDependenciesTests
*/

#include <gtest/gtest.h>

#include <memory>

#include "FakeBackend.hpp"
#include "plugins/audio/AudioPlugin.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/particle/ParticlePlugin.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/shader/ShaderPlugin.hpp"
#include "plugins/sprite/SpriteDrawPlugin.hpp"
#include "plugins/sprite/SpritePlugin.hpp"
#include "plugins/text/TextPlugin.hpp"
#include "plugins/tilemap/TilemapPlugin.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/exceptions/PluginException.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"

namespace {
using rtype::ecs::App;
using rtype::ecs::RunMode;
using rtype::ecs::exceptions::PluginException;
using rtype::plugins::graphics::GraphicsPlugin;
using rtype::plugins::input::InputPlugin;
using rtype::plugins::shader::ShaderPlugin;
using rtype::plugins::sprite::SpriteDrawPlugin;
using rtype::plugins::sprite::SpritePlugin;

/// @brief Adds what every plugin that draws needs, and nothing else.
void addGraphics(App& app) {
    app.addPlugin(rtype::plugins::transform::TransformPlugin{});
    app.addPlugin(InputPlugin{});
    app.addPlugin(GraphicsPlugin{
        rtype::test::plugins::makeFakeRegistry(std::make_shared<rtype::test::plugins::FakeBackendState>()),
        rtype::test::plugins::makeFakeSettings()});
}

TEST(PluginDependencies, TheGraphicsPluginRequiresTheInputPlugin) {
    App app{RunMode::kClient};
    EXPECT_THROW(app.addPlugin(GraphicsPlugin{
                     rtype::test::plugins::makeFakeRegistry(std::make_shared<rtype::test::plugins::FakeBackendState>()),
                     rtype::test::plugins::makeFakeSettings()}),
                 PluginException);
}

TEST(PluginDependencies, TheGraphicsPluginAddsNoPlugin) {
    App app{RunMode::kClient};
    addGraphics(app);
    EXPECT_FALSE(app.hasPlugin(ShaderPlugin::kName));
    EXPECT_FALSE(app.hasPlugin(SpritePlugin::kName));
    EXPECT_FALSE(app.hasPlugin(rtype::plugins::tilemap::TilemapPlugin::kName));
    EXPECT_FALSE(app.hasPlugin(rtype::plugins::text::TextPlugin::kName));
    EXPECT_FALSE(app.hasPlugin(rtype::plugins::particle::ParticlePlugin::kName));
}

TEST(PluginDependencies, TheGraphicsPluginRunsAlone) {
    App app{RunMode::kClient};
    addGraphics(app);
    EXPECT_NO_THROW(app.runFrame());
}

TEST(PluginDependencies, TheShaderPluginRequiresTheGraphicsPlugin) {
    App app{RunMode::kClient};
    EXPECT_THROW(app.addPlugin(ShaderPlugin{}), PluginException);
}

TEST(PluginDependencies, TheTilemapPluginRunsWithoutTheOtherPluginsThatDraw) {
    App app{RunMode::kClient};
    addGraphics(app);
    app.addPlugin(rtype::plugins::tilemap::TilemapPlugin{});
    EXPECT_NO_THROW(app.runFrame());
}

TEST(PluginDependencies, TheTextPluginRunsWithoutTheTilemapPlugin) {
    App app{RunMode::kClient};
    addGraphics(app);
    app.addPlugin(ShaderPlugin{});
    app.addPlugin(rtype::plugins::text::TextPlugin{});
    EXPECT_NO_THROW(app.runFrame());
}

TEST(PluginDependencies, TheParticlePluginRunsWithoutTheTextPlugin) {
    App app{RunMode::kClient};
    addGraphics(app);
    app.addPlugin(ShaderPlugin{});
    app.addPlugin(rtype::plugins::particle::ParticlePlugin{});
    EXPECT_NO_THROW(app.runFrame());
}

TEST(PluginDependencies, TheSpriteDrawPluginRunsWithoutTheOtherPluginsThatDraw) {
    App app{RunMode::kClient};
    addGraphics(app);
    app.addPlugin(ShaderPlugin{});
    app.addPlugin(SpritePlugin{});
    app.addPlugin(SpriteDrawPlugin{});
    EXPECT_NO_THROW(app.runFrame());
}

TEST(PluginDependencies, TheSpriteDrawPluginRequiresTheSpritePlugin) {
    App app{RunMode::kClient};
    addGraphics(app);
    app.addPlugin(ShaderPlugin{});
    EXPECT_THROW(app.addPlugin(SpriteDrawPlugin{}), PluginException);
}

TEST(PluginDependencies, TheSpritePluginRunsAloneOnAServer) {
    App app{RunMode::kServer};
    app.addPlugin(SpritePlugin{});
    EXPECT_NO_THROW(app.runFrame());
}

TEST(PluginDependencies, ThePhysicsPluginRunsWithoutAnythingThatDraws) {
    App app{RunMode::kServer};
    app.addPlugin(rtype::plugins::transform::TransformPlugin{});
    app.addPlugin(rtype::plugins::physics::PhysicsPlugin{});
    EXPECT_NO_THROW(app.runFrame());
}

TEST(PluginDependencies, ThePhysicsPluginRequiresTheTransformPlugin) {
    App app{RunMode::kServer};
    EXPECT_THROW(app.addPlugin(rtype::plugins::physics::PhysicsPlugin{}), PluginException);
}
}  // namespace
