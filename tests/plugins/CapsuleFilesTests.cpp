/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleFilesTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

#include "plugins/AssetDirectory.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleFiles.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/CapsuleSettings.hpp"
#include "plugins/capsule/CapsuleSpawn.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"
#include "rtype/core/config/Settings.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::capsule_files {
struct Health {
    std::int32_t value{1};
};
RTYPE_ECS_COMPONENT(Health, "test.files.Health", value);

struct Armor {
    std::int32_t value{0};
    std::int32_t extra{0};
};
RTYPE_ECS_COMPONENT(Armor, "test.files.Armor", value, extra);
}  // namespace rtype::test::capsule_files

namespace capsule = rtype::plugins::capsule;
namespace tf = rtype::test::capsule_files;

namespace {
constexpr std::string_view kTurret = R"([node path="."]
test.files.Health = { value = 5 }
test.files.Armor = { value = 1, extra = 2 }
)";
constexpr std::string_view kBoss = R"([node path="."]
test.files.Health = { value = 50 }
[node path="Turret" instance="actors/turret.actor"]
test.files.Armor = { extra = 9 }
)";

/// @brief A game directory of its own, removed when the test ends, with the test components registered.
class CapsuleFilesTest : public ::testing::Test {
  protected:
    void SetUp() override {
        std::filesystem::create_directories(_game / "actors");
        static_cast<void>(world.getComponents().registerType<tf::Health>());
        static_cast<void>(world.getComponents().registerType<tf::Armor>());
    }
    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(_game, error);
    }

    /// @brief Writes the file `path` of the game's assets, dated a second after the last one so that it reads as
    /// changed.
    void write(std::string_view path, std::string_view text) {
        const std::filesystem::path file = _game / path;
        std::filesystem::create_directories(file.parent_path());
        std::ofstream{file, std::ios::binary} << text;
        _time += std::chrono::seconds{1};
        std::filesystem::last_write_time(file, _time);
    }

    /// @return A capsule's node value of `T`, or nullptr.
    template <typename T>
    const T* valueOf(std::string_view path, std::size_t node) const {
        const capsule::Capsule* found = capsules.find(path);
        return found != nullptr ? found->getNodes()[node].values.get<T>(world.getComponents()) : nullptr;
    }

    std::filesystem::path _game{std::filesystem::temp_directory_path() /
                                std::format("rtype_capsule_files_{:08x}", std::random_device{}())};
    std::filesystem::file_time_type _time{std::filesystem::file_time_type::clock::now()};  ///< Of the last write.
    rtype::ecs::World world;                                                               ///< Holds the components.
    capsule::Capsules capsules;  ///< What the files are read into.
};
}  // namespace

TEST_F(CapsuleFilesTest, ReadsEveryCapsuleFileAnInstancedOneFirst) {
    write("actors/boss.actor", kBoss);  // sorted before the turret it instances
    write("actors/turret.actor", kTurret);
    write("scenes/level.scene", R"([node path="Boss" instance="actors/boss.actor"])"
                                "\n");  // not a root first: refused below
    write("scenes/empty.capsule", "");
    write("sprites/ship.png", "not a capsule");
    std::filesystem::remove(_game / "scenes/level.scene");
    capsule::CapsuleFiles files{_game, world.getComponents()};
    const std::vector<capsule::CapsuleId> read = files.loadAll(capsules);
    EXPECT_EQ(read.size(), 3U);
    EXPECT_EQ(capsules.getCount(), 3U);
    const tf::Armor* armor = valueOf<tf::Armor>("actors/boss.actor", 1);
    ASSERT_NE(armor, nullptr);
    EXPECT_EQ(armor->value, 1);  // the turret's, read first
    EXPECT_EQ(armor->extra, 9);  // the boss's override
    EXPECT_NE(capsules.find("scenes/empty.capsule"), nullptr);
}

TEST_F(CapsuleFilesTest, RefusesAFileThatCannotBeRead) {
    write("actors/broken.actor", "[node path=\".\"]\ntest.files.Health = { value = \"x\" }\n");
    capsule::CapsuleFiles files{_game, world.getComponents()};
    try {
        static_cast<void>(files.loadAll(capsules));
        FAIL() << "no CapsuleException";
    } catch (const capsule::exceptions::CapsuleException& error) {
        EXPECT_TRUE(std::string{error.what()}.starts_with("actors/broken.actor:2:31:")) << error.what();
    }
}

TEST_F(CapsuleFilesTest, ReadsASavedFileAgainWithTheFilesThatInstanceIt) {
    write("actors/turret.actor", kTurret);
    write("actors/boss.actor", kBoss);
    capsule::CapsuleFiles files{_game, world.getComponents()};
    static_cast<void>(files.loadAll(capsules));
    const auto start = capsule::CapsuleFiles::Clock::now();
    write("actors/turret.actor", "[node path=\".\"]\ntest.files.Armor = { value = 4 }\n");
    EXPECT_TRUE(files.update(capsules, start).empty());  // before the interval
    const std::vector<capsule::CapsuleId> reread = files.update(capsules, start + capsule::CapsuleFiles::kInterval * 2);
    EXPECT_EQ(reread.size(), 2U);  // the turret, and the boss whose override starts from it
    EXPECT_EQ(capsules.find("actors/turret.actor")->getVersion(), 1U);
    EXPECT_EQ(capsules.find("actors/boss.actor")->getVersion(), 1U);
    EXPECT_EQ(valueOf<tf::Armor>("actors/boss.actor", 1)->value, 4);  // the turret's new value
    EXPECT_EQ(valueOf<tf::Armor>("actors/boss.actor", 1)->extra, 9);
}

TEST_F(CapsuleFilesTest, KeepsTheOldVersionOfAFileSavedBroken) {
    write("actors/turret.actor", kTurret);
    capsule::CapsuleFiles files{_game, world.getComponents()};
    static_cast<void>(files.loadAll(capsules));
    write("actors/turret.actor", "[node path=\".\"]\ntest.files.Health = { value = oops }\n");
    const auto later = capsule::CapsuleFiles::Clock::now() + capsule::CapsuleFiles::kInterval * 2;
    EXPECT_TRUE(files.update(capsules, later).empty());
    EXPECT_EQ(capsules.find("actors/turret.actor")->getVersion(), 0U);
    EXPECT_EQ(valueOf<tf::Health>("actors/turret.actor", 0)->value, 5);
}

TEST_F(CapsuleFilesTest, ReadsAFileThatAppears) {
    capsule::CapsuleFiles files{_game, world.getComponents()};
    static_cast<void>(files.loadAll(capsules));
    write("actors/turret.actor", kTurret);
    const auto later = capsule::CapsuleFiles::Clock::now() + capsule::CapsuleFiles::kInterval * 2;
    EXPECT_EQ(files.update(capsules, later).size(), 1U);
    EXPECT_NE(capsules.find("actors/turret.actor"), nullptr);
}

TEST_F(CapsuleFilesTest, TheGameLoadsItsFilesAndReloadsWhatTheySpawned) {
    write("actors/turret.actor", kTurret);
    write("scenes/level.scene", "[node path=\".\"]\n[node path=\"Turret\" instance=\"actors/turret.actor\"]\n");
    rtype::ecs::App app;
    app.insertResource(rtype::plugins::AssetDirectory{.path = _game});
    static_cast<void>(app.getWorld().getComponents().registerType<tf::Health>());
    static_cast<void>(app.getWorld().getComponents().registerType<tf::Armor>());
    app.addPlugin(capsule::CapsulePlugin{capsule::CapsuleSettings{.hotReload = true}});
    app.finishPlugins();
    const rtype::ecs::Entity level = capsule::loadCapsule(app.getWorld(), "scenes/level.scene");
    const rtype::ecs::Entity turret = app.getWorld().getComponents().get<rtype::ecs::Children>(level)->entities.at(0);
    EXPECT_EQ(app.getWorld().getComponents().get<tf::Health>(turret)->value, 5);

    write("actors/turret.actor", "[node path=\".\"]\ntest.files.Health = { value = 8 }\n");
    std::this_thread::sleep_for(capsule::CapsuleFiles::kInterval + std::chrono::milliseconds{50});
    app.runFrame();
    ASSERT_TRUE(app.getWorld().isAlive(level));
    const rtype::ecs::Entity reloaded = app.getWorld().getComponents().get<rtype::ecs::Children>(level)->entities.at(0);
    EXPECT_EQ(app.getWorld().getComponents().get<tf::Health>(reloaded)->value, 8);
    EXPECT_EQ(app.getWorld().getComponents().get<capsule::CapsuleInstance>(reloaded)->version, 1U);
}

TEST(CapsuleSettings, ReadsHotReload) {
    rtype::core::config::Settings settings;
    settings.set("capsule.hotReload", true);
    EXPECT_TRUE(capsule::CapsuleSettings::fromSettings(settings).hotReload);
    settings.set("capsule.hotReload", false);
    EXPECT_FALSE(capsule::CapsuleSettings::fromSettings(settings).hotReload);
}

TEST(CapsuleSettings, RejectsAnUnknownKey) {
    rtype::core::config::Settings settings;
    settings.set("capsule.hotReloadd", true);
    EXPECT_ANY_THROW(static_cast<void>(capsule::CapsuleSettings::fromSettings(settings)));
}
