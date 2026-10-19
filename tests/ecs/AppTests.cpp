/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AppTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string_view>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/AppExit.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/ecs/app/PluginGroup.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/Time.hpp"
#include "rtype/ecs/time/TimeClock.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"

namespace rtype::test::app {
/// @brief Frames run so far, and when the counting system asks to stop.
struct Frames {
    int count;
    int exitAfter;  ///< Frames to run before sending AppExit.
    int code;       ///< The code to send.
};
RTYPE_ECS_RESOURCE(Frames, "test.app.Frames", count, exitAfter, code);

/// @brief Startup runs, to check it runs once and first.
struct Started {
    int count;
    int framesBefore;  ///< Frames counted when Startup ran.
};
RTYPE_ECS_RESOURCE(Started, "test.app.Started", count, framesBefore);

struct FixedRuns {
    int count;
};
RTYPE_ECS_RESOURCE(FixedRuns, "test.app.FixedRuns", count);
}  // namespace rtype::test::app

namespace {
using rtype::ecs::App;
using rtype::ecs::AppExit;
using rtype::ecs::EventWriter;
using rtype::ecs::FixedTimestep;
using rtype::ecs::MinimalPlugins;
using rtype::ecs::Phase;
using rtype::ecs::Plugin;
using rtype::ecs::PluginGroup;
using rtype::ecs::Res;
using rtype::ecs::ResMut;
using rtype::ecs::RunMode;
using rtype::ecs::Side;
using rtype::ecs::Time;
using rtype::ecs::TimeClock;
using rtype::ecs::TimePlugin;
using rtype::test::app::FixedRuns;
using rtype::test::app::Frames;
using rtype::test::app::Started;

void countFrame(ResMut<Frames> frames, EventWriter<AppExit> exit) {
    ++frames->count;
    if (frames->count == frames->exitAfter) {
        exit.send(AppExit{.code = frames->code});
    }
}

void markStarted(ResMut<Started> started, Res<Frames> frames) {
    ++started->count;
    started->framesBefore = frames->count;
}

void countFixed(ResMut<FixedRuns> runs) { ++runs->count; }

/// @return The App's clock as a TimeClock, or nullptr if it is another clock or none.
const TimeClock* timeClockOf(const App& app) { return dynamic_cast<const TimeClock*>(app.getSchedules().getClock()); }

void sendTwoExits(EventWriter<AppExit> exit) {
    exit.send(AppExit{.code = 3});
    exit.send(AppExit{.code = 4});
}

/// @brief Counts frames from Update and sends AppExit after `exitAfter` of them.
class FramesPlugin : public Plugin {
  public:
    FramesPlugin(int exitAfter, int code) noexcept : _exitAfter{exitAfter}, _code{code} {}
    ~FramesPlugin() override = default;
    FramesPlugin(const FramesPlugin& other) = default;
    FramesPlugin& operator=(const FramesPlugin& other) = default;
    FramesPlugin(FramesPlugin&& other) noexcept = default;
    FramesPlugin& operator=(FramesPlugin&& other) noexcept = default;

    [[nodiscard]] std::string_view getName() const noexcept override { return "test.frames"; }

    void build(App& app) override {
        app.insertResource(Frames{.count = 0, .exitAfter = _exitAfter, .code = _code});
        app.insertResource(Started{.count = 0, .framesBefore = -1});
        app.addSystem(Phase::kStartup, "test.markStarted", &markStarted);
        app.addSystem(Phase::kUpdate, "test.countFrame", &countFrame).setSide(Side::kBoth);
    }

  private:
    int _exitAfter;  ///< Frames to run before sending AppExit.
    int _code;       ///< The code to send.
};

/// @brief Counts its builds in an int it does not own, and adds the plugins named by `adds`.
class CountingPlugin : public Plugin {
  public:
    CountingPlugin(std::string_view name, int* builds, std::string_view adds = {}) noexcept
        : _name{name}, _builds{builds}, _adds{adds} {}
    ~CountingPlugin() override = default;
    CountingPlugin(const CountingPlugin& other) = default;
    CountingPlugin& operator=(const CountingPlugin& other) = default;
    CountingPlugin(CountingPlugin&& other) noexcept = default;
    CountingPlugin& operator=(CountingPlugin&& other) noexcept = default;

    [[nodiscard]] std::string_view getName() const noexcept override { return _name; }

    void build(App& app) override {
        ++*_builds;
        if (!_adds.empty()) {
            app.addPlugin(CountingPlugin{_adds, _builds, _name});  // which adds this one back
        }
    }

  private:
    std::string_view _name;  ///< The plugin's name.
    int* _builds;            ///< Incremented by each build.
    std::string_view _adds;  ///< The plugin it adds when built; empty for none.
};

/// @brief Adds two counting plugins, one of them twice.
class CountingGroup : public PluginGroup {
  public:
    explicit CountingGroup(int* builds) noexcept : _builds{builds} {}
    ~CountingGroup() override = default;
    CountingGroup(const CountingGroup& other) = default;
    CountingGroup& operator=(const CountingGroup& other) = default;
    CountingGroup(CountingGroup&& other) noexcept = default;
    CountingGroup& operator=(CountingGroup&& other) noexcept = default;

    void build(App& app) override {
        app.addPlugin(CountingPlugin{"test.a", _builds})
            .addPlugin(CountingPlugin{"test.b", _builds})
            .addPlugin(CountingPlugin{"test.a", _builds});
    }

  private:
    int* _builds;  ///< Given to every plugin of the group.
};
}  // namespace

TEST(App, KeepsItsRunMode) {
    const App app{RunMode::kServer};
    EXPECT_EQ(app.getSchedules().getRunMode(), RunMode::kServer);
    EXPECT_EQ(App{}.getSchedules().getRunMode(), RunMode::kStandalone);
}

TEST(App, BuildsAPluginWhenItIsAdded) {
    App app;
    app.addPlugin(FramesPlugin{3, 0});
    EXPECT_TRUE(app.hasPlugin("test.frames"));
    EXPECT_FALSE(app.hasPlugin("test.other"));
    EXPECT_TRUE(app.getWorld().getResources().has<Frames>());  // built at once, not when the App runs
    EXPECT_TRUE(app.getSchedules().getSchedule(Phase::kUpdate).contains("test.countFrame"));
}

TEST(App, IgnoresAPluginAddedTwice) {
    App app;
    int builds = 0;
    app.addPlugin(CountingPlugin{"test.a", &builds}).addPlugin(CountingPlugin{"test.a", &builds});
    EXPECT_EQ(builds, 1);
}

TEST(App, BuildsAPluginOnceWhenPluginsAddEachOther) {
    App app;
    int builds = 0;
    app.addPlugin(CountingPlugin{"test.a", &builds, "test.b"});  // a adds b, which adds a back
    EXPECT_EQ(builds, 2);
    EXPECT_TRUE(app.hasPlugin("test.a"));
    EXPECT_TRUE(app.hasPlugin("test.b"));
}

TEST(App, AcceptsAPluginByPointer) {
    App app;
    int builds = 0;
    app.addPlugin(std::make_unique<CountingPlugin>("test.a", &builds));
    EXPECT_EQ(builds, 1);
    EXPECT_TRUE(app.hasPlugin("test.a"));
}

TEST(App, AddsPluginGroups) {
    App app;
    int builds = 0;
    app.addPlugins(CountingGroup{&builds});
    EXPECT_EQ(builds, 2);  // "test.a" was in the group twice
    EXPECT_TRUE(app.hasPlugin("test.a"));
    EXPECT_TRUE(app.hasPlugin("test.b"));
}

TEST(App, RunsFramesUntilAppExit) {
    App app;
    app.addPlugin(FramesPlugin{5, 7});
    EXPECT_EQ(app.run(), 7);
    EXPECT_EQ(app.getWorld().getResources().require<Frames>().count, 5);  // exactly the frames asked for
    const Started& started = app.getWorld().getResources().require<Started>();
    EXPECT_EQ(started.count, 1);
    EXPECT_EQ(started.framesBefore, 0);  // Startup runs once, before the first frame's Update
}

TEST(App, ReturnsTheCodeOfTheFirstExit) {
    App app;
    app.addSystem(Phase::kPostUpdate, "test.sendTwoExits", &sendTwoExits).setSide(Side::kBoth);
    EXPECT_EQ(app.run(), 3);
}

TEST(App, RunsOnlyTheSystemsOfItsMode) {
    App app{RunMode::kClient};
    app.addPlugin(FramesPlugin{1, 0});
    app.addSystem(Phase::kPostUpdate, "test.serverExit", &sendTwoExits);  // kServer: never runs on a client
    EXPECT_EQ(app.run(), 0);                                              // stopped by the plugin, after 1 frame
}

TEST(App, RunsOneFrameAtATime) {
    App app;
    app.addPlugin(FramesPlugin{100, 0});
    app.runFrame();
    app.runFrame();
    EXPECT_EQ(app.getWorld().getResources().require<Frames>().count, 2);
    EXPECT_EQ(app.getWorld().getResources().require<Started>().count, 1);
}

TEST(App, AddsTheDefaultTimePluginBeforeItsFirstFrame) {
    App app;
    EXPECT_FALSE(app.hasPlugin(TimePlugin::kName));
    app.runFrame();
    EXPECT_TRUE(app.hasPlugin(TimePlugin::kName));
    EXPECT_TRUE(app.getWorld().getResources().has<Time>());
    const TimeClock* clock = timeClockOf(app);
    ASSERT_NE(clock, nullptr);
    EXPECT_EQ(clock->getFixedTimestep().getRate(), 60U);
    EXPECT_FALSE(clock->getManualFrame());
}

TEST(App, KeepsAConfiguredTimePlugin) {
    App app;
    app.addPlugin(TimePlugin::manual(std::chrono::milliseconds{50}, FixedTimestep{30, 5}));
    app.runFrame();
    const TimeClock* clock = timeClockOf(app);
    ASSERT_NE(clock, nullptr);
    EXPECT_EQ(clock->getFixedTimestep().getRate(), 30U);
    EXPECT_EQ(clock->getManualFrame(), std::chrono::milliseconds{50});
}

TEST(App, GetsTimeFromMinimalPlugins) {
    App app;
    app.addPlugins(MinimalPlugins{});
    EXPECT_TRUE(app.hasPlugin(TimePlugin::kName));
    EXPECT_NE(timeClockOf(app), nullptr);
}

TEST(App, KeepsATimePluginAddedBeforeMinimalPlugins) {
    App app;
    app.addPlugin(TimePlugin{FixedTimestep{30, 5}}).addPlugins(MinimalPlugins{});
    const TimeClock* clock = timeClockOf(app);
    ASSERT_NE(clock, nullptr);
    EXPECT_EQ(clock->getFixedTimestep().getRate(), 30U);  // the group's default one was skipped
}

TEST(App, RunsAFixedNumberOfFramesAndTicksWithManualTime) {
    App app;
    app.addPlugin(TimePlugin::manual(std::chrono::milliseconds{50})).addPlugin(FramesPlugin{4, 0});
    app.insertResource(FixedRuns{.count = 0});
    app.addSystem(Phase::kFixedUpdate, "test.countFixed", &countFixed);
    EXPECT_EQ(app.run(), 0);
    EXPECT_EQ(app.getWorld().getResources().require<Frames>().count, 4);
    EXPECT_EQ(app.getWorld().getResources().require<FixedRuns>().count, 12);  // 3 ticks in each 50 ms frame
    EXPECT_EQ(app.getWorld().getResources().require<Time>().tick, 12U);
}
