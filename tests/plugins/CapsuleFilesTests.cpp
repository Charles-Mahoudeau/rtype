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
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "plugins/AssetDirectory.hpp"
#include "plugins/HotReload.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleFiles.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/CapsuleReloadable.hpp"
#include "plugins/capsule/CapsuleSpawn.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"
#include "plugins/capsule/ReloadedCapsules.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"
#include "rtype/core/reload/HotReloader.hpp"
#include "rtype/core/reload/ReloadOutcome.hpp"
#include "rtype/core/reload/ReloadReport.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/command/Commands.hpp"
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

    /// @brief Polls the reloader twice, as its debounce wants the same stamp two times.
    static std::vector<rtype::core::reload::ReloadReport> settle(rtype::core::reload::HotReloader& reloader) {
        static_cast<void>(reloader.poll());
        return reloader.poll();
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
    write("actors/turret.actor", "[node path=\".\"]\ntest.files.Armor = { value = 4 }\n");
    const std::vector<std::string> saved{"actors/turret.actor"};
    const capsule::CapsuleFiles::Reread reread = files.reload(capsules, saved);
    EXPECT_EQ(reread.capsules.size(), 2U);  // the turret, and the boss whose override starts from it
    EXPECT_TRUE(reread.errors.empty());
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
    const std::vector<std::string> saved{"actors/turret.actor"};
    const capsule::CapsuleFiles::Reread reread = files.reload(capsules, saved);
    EXPECT_TRUE(reread.capsules.empty());
    ASSERT_EQ(reread.errors.size(), 1U);
    EXPECT_TRUE(reread.errors[0].starts_with("actors/turret.actor:2:")) << reread.errors[0];
    EXPECT_EQ(capsules.find("actors/turret.actor")->getVersion(), 0U);
    EXPECT_EQ(valueOf<tf::Health>("actors/turret.actor", 0)->value, 5);
}

TEST_F(CapsuleFilesTest, ReadsAFileThatAppearsAndSkipsOtherFiles) {
    capsule::CapsuleFiles files{_game, world.getComponents()};
    static_cast<void>(files.loadAll(capsules));
    write("actors/turret.actor", kTurret);
    write("sprites/ship.png", "not a capsule");
    const std::vector<std::string> added{"actors/turret.actor", "sprites/ship.png"};
    EXPECT_EQ(files.reload(capsules, added).capsules.size(), 1U);
    EXPECT_NE(capsules.find("actors/turret.actor"), nullptr);
}

TEST_F(CapsuleFilesTest, TheReloadableReadsWhatTheReloaderSees) {
    write("actors/turret.actor", kTurret);
    write("actors/boss.actor", kBoss);
    capsule::CapsuleFiles files{_game, world.getComponents()};
    static_cast<void>(files.loadAll(capsules));
    capsule::ReloadedCapsules reloaded;
    std::vector<std::string> log;
    rtype::core::reload::HotReloader reloader{_game.parent_path(),
                                              [&](std::string_view line) { log.emplace_back(line); }};
    const std::filesystem::path assets = _game.filename();  // the assets, in the reloader's root
    auto reloadable = std::make_unique<capsule::CapsuleReloadable>(&files, &capsules, &reloaded, assets);
    EXPECT_EQ(reloadable->name(), "capsules");
    ASSERT_EQ(reloadable->watched().size(), 1U);
    EXPECT_EQ(reloadable->watched()[0].path, assets);
    EXPECT_EQ(reloadable->watched()[0].extensions, (std::vector<std::string>{".capsule", ".scene", ".actor"}));
    reloader.add(std::move(reloadable));

    write("actors/turret.actor", "[node path=\".\"]\ntest.files.Armor = { value = 4 }\n");
    write("sprites/ship.png", "not followed");
    std::vector<rtype::core::reload::ReloadReport> reports = settle(reloader);
    ASSERT_EQ(reports.size(), 1U);
    EXPECT_EQ(reports[0].outcome.status, rtype::core::reload::ReloadOutcome::Status::kApplied);
    EXPECT_EQ(reloaded.capsules.size(), 2U);  // the turret and the boss, for the next frame
    EXPECT_EQ(capsules.find("actors/boss.actor")->getVersion(), 1U);

    write("actors/turret.actor", "[node path=\".\"]\ntest.files.Armor = { value = oops }\n");
    reports = settle(reloader);
    ASSERT_EQ(reports.size(), 1U);
    EXPECT_EQ(reports[0].outcome.status, rtype::core::reload::ReloadOutcome::Status::kFailed);
    EXPECT_NE(reports[0].outcome.message.find("actors/turret.actor:2:"), std::string::npos)
        << reports[0].outcome.message;
    EXPECT_EQ(capsules.find("actors/turret.actor")->getVersion(), 1U);  // kept
    EXPECT_EQ(log.size(), 2U);
}

TEST_F(CapsuleFilesTest, TheGameLoadsItsFilesAndReloadsWhatTheySpawned) {
    write("actors/turret.actor", kTurret);
    write("scenes/level.scene", "[node path=\".\"]\n[node path=\"Turret\" instance=\"actors/turret.actor\"]\n");
    rtype::core::reload::HotReloader reloader{_game.parent_path(), [](std::string_view /*line*/) {}};
    rtype::ecs::App app;
    app.insertResource(rtype::plugins::AssetDirectory{.path = _game});
    app.insertResource(rtype::plugins::HotReload{.reloader = &reloader});
    static_cast<void>(app.getWorld().getComponents().registerType<tf::Health>());
    static_cast<void>(app.getWorld().getComponents().registerType<tf::Armor>());
    app.addPlugin(capsule::CapsulePlugin{});
    app.finishPlugins();
    EXPECT_EQ(reloader.size(), 1U);
    rtype::ecs::Commands commands{app.getWorld()};
    const rtype::ecs::Entity level =
        capsule::loadCapsule(commands, app.getWorld().getResources().require<capsule::Capsules>(),
                             *app.getWorld().getResources().getMut<capsule::LoadedCapsule>(),
                             capsule::CapsuleId::fromPath("scenes/level.scene"))
            .getEntity();
    commands.flush();
    const rtype::ecs::Entity turret = app.getWorld().getComponents().get<rtype::ecs::Children>(level)->entities.at(0);
    EXPECT_EQ(app.getWorld().getComponents().get<tf::Health>(turret)->value, 5);

    write("actors/turret.actor", "[node path=\".\"]\ntest.files.Health = { value = 8 }\n");
    EXPECT_EQ(settle(reloader).size(), 1U);
    app.runFrame();
    ASSERT_TRUE(app.getWorld().isAlive(level));
    const rtype::ecs::Entity reloaded = app.getWorld().getComponents().get<rtype::ecs::Children>(level)->entities.at(0);
    EXPECT_EQ(app.getWorld().getComponents().get<tf::Health>(reloaded)->value, 8);
    EXPECT_EQ(app.getWorld().getComponents().get<capsule::CapsuleInstance>(reloaded)->version, 1U);
    EXPECT_TRUE(app.getWorld().getResources().get<capsule::ReloadedCapsules>()->capsules.empty());
}

TEST_F(CapsuleFilesTest, WithoutHotReloadTheGameOnlyLoadsItsFiles) {
    write("actors/turret.actor", kTurret);
    rtype::ecs::App app;
    app.insertResource(rtype::plugins::AssetDirectory{.path = _game});
    static_cast<void>(app.getWorld().getComponents().registerType<tf::Health>());
    static_cast<void>(app.getWorld().getComponents().registerType<tf::Armor>());
    app.addPlugin(capsule::CapsulePlugin{});
    app.finishPlugins();
    EXPECT_NE(app.getWorld().getResources().get<capsule::Capsules>()->find("actors/turret.actor"), nullptr);
    EXPECT_EQ(app.getWorld().getResources().get<capsule::ReloadedCapsules>(), nullptr);
}
