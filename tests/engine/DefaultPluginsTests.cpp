/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DefaultPluginsTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string_view>

#include "core/config/Settings.hpp"
#include "plugins/audio/AudioPlugin.hpp"
#include "plugins/audio/AudioRegistry.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/particle/ParticlePlugin.hpp"
#include "plugins/sprite/SpritePlugin.hpp"
#include "plugins/text/TextPlugin.hpp"
#include "plugins/tilemap/TilemapPlugin.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/time/TimeClock.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/engine/DefaultPlugins.hpp"

namespace {
/// @return The default plugins, with empty registries: building them only registers, nothing is opened.
rtype::engine::DefaultPlugins makeDefaults() {
    return rtype::engine::DefaultPlugins{std::make_unique<rtype::plugins::backend::BackendRegistry>(),
                                         std::make_unique<rtype::plugins::audio::AudioRegistry>(),
                                         rtype::core::config::Settings{}};
}
}  // namespace

TEST(DefaultPlugins, AddTimeTheBackendAndAudio) {
    rtype::ecs::App app{rtype::ecs::RunMode::kClient};
    app.addPlugins(makeDefaults());
    EXPECT_TRUE(app.hasPlugin(rtype::ecs::TimePlugin::kName));
    EXPECT_TRUE(app.hasPlugin(rtype::plugins::backend::BackendPlugin::kName));
    EXPECT_TRUE(app.hasPlugin(rtype::plugins::audio::AudioPlugin::kName));
}

TEST(DefaultPlugins, BringWhatTheBackendAdds) {
    rtype::ecs::App app{rtype::ecs::RunMode::kClient};
    app.addPlugins(makeDefaults());
    for (const std::string_view name :
         {rtype::plugins::input::InputPlugin::kName, rtype::plugins::transform::TransformPlugin::kName,
          rtype::plugins::sprite::SpritePlugin::kName, rtype::plugins::tilemap::TilemapPlugin::kName,
          rtype::plugins::text::TextPlugin::kName, rtype::plugins::particle::ParticlePlugin::kName}) {
        EXPECT_TRUE(app.hasPlugin(name)) << name;
    }
}

TEST(DefaultPlugins, KeepAPluginTheGameAddedFirst) {
    rtype::ecs::App app{rtype::ecs::RunMode::kClient};
    app.addPlugin(rtype::ecs::TimePlugin::manual(std::chrono::milliseconds{20}));
    app.addPlugins(makeDefaults());  // its MinimalPlugins' TimePlugin is skipped: already added
    const auto* clock = dynamic_cast<const rtype::ecs::TimeClock*>(app.getSchedules().getClock());
    ASSERT_NE(clock, nullptr);
    EXPECT_EQ(clock->getManualFrame(), std::chrono::milliseconds{20});  // still the game's manual clock
    EXPECT_TRUE(app.hasPlugin(rtype::plugins::backend::BackendPlugin::kName));
}
