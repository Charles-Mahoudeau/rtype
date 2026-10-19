/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** BackendPluginTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <vector>

#include "FakeBackend.hpp"
#include "engine/backend/BackendRegistry.hpp"
#include "engine/event/Event.hpp"
#include "engine/input/Control.hpp"
#include "engine/input/Key.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/BackendResources.hpp"
#include "plugins/backend/WindowEvents.hpp"
#include "plugins/input/Input.hpp"
#include "plugins/input/InputEvents.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"

namespace {
/// @brief An App with the BackendPlugin over the fake platform and renderer.
class BackendApp {
  public:
    explicit BackendApp(rtype::ecs::RunMode mode = rtype::ecs::RunMode::kClient)
        : _state{std::make_shared<rtype::test::plugins::FakeBackendState>()}, _app{mode} {
        _app.addPlugin(rtype::plugins::backend::BackendPlugin{rtype::test::plugins::makeFakeRegistry(_state),
                                                              rtype::test::plugins::makeFakeSettings()});
    }
    ~BackendApp() = default;
    BackendApp(const BackendApp& other) = delete;
    BackendApp& operator=(const BackendApp& other) = delete;
    BackendApp(BackendApp&& other) = delete;
    BackendApp& operator=(BackendApp&& other) = delete;

    [[nodiscard]] rtype::ecs::App& app() noexcept { return _app; }
    [[nodiscard]] rtype::test::plugins::FakeBackendState& state() noexcept { return *_state; }

  private:
    std::shared_ptr<rtype::test::plugins::FakeBackendState> _state;  ///< Drives the fakes.
    rtype::ecs::App _app;                                            ///< The App under test.
};
}  // namespace

TEST(BackendPlugin, CreatesNothingBeforeTheFirstFrame) {
    BackendApp app;
    EXPECT_EQ(app.state().initCount, 0);
    EXPECT_TRUE(app.app().getWorld().getResources().has<rtype::engine::backend::Backend>());
    EXPECT_EQ(app.app().getWorld().getResources().get<rtype::engine::backend::Backend>()->platform, nullptr);
}

TEST(BackendPlugin, CreatesTheBackendAtStartup) {
    BackendApp app;
    app.app().runFrame();
    EXPECT_EQ(app.state().initCount, 1);
    const auto* backend = app.app().getWorld().getResources().get<rtype::engine::backend::Backend>();
    ASSERT_NE(backend, nullptr);
    EXPECT_NE(backend->platform, nullptr);
    EXPECT_NE(backend->renderer, nullptr);
    EXPECT_EQ(backend->platform->getTitle(), "Test");
}

TEST(BackendPlugin, FramesEveryFrameWithTheClearColor) {
    BackendApp app;
    app.app().getWorld().getResources().insert(rtype::plugins::backend::ClearColor{.value = {1.0F, 0.0F, 0.0F, 1.0F}});
    app.app().runFrame();
    app.app().runFrame();
    EXPECT_EQ(app.state().beginFrames, 2);
    EXPECT_EQ(app.state().endFrames, 2);
    EXPECT_EQ(app.state().clearColor, (rtype::engine::graphics::Color{1.0F, 0.0F, 0.0F, 1.0F}));
}

TEST(BackendPlugin, SendsThePlatformEventsAsEcsEvents) {
    BackendApp app;
    app.state().frames.push_back({rtype::engine::event::KeyPressed{.key = rtype::engine::input::Key::kA},
                                  rtype::engine::event::TextEntered{.codepoint = U'a'}});
    app.app().runFrame();
    EXPECT_EQ(app.app().getWorld().getEvents().getCount<rtype::engine::event::KeyPressed>(), 1U);
    EXPECT_EQ(app.app().getWorld().getEvents().getCount<rtype::engine::event::TextEntered>(), 1U);
}

TEST(BackendPlugin, ResizesTheRenderer) {
    BackendApp app;
    app.state().frames.push_back({rtype::engine::event::Resized{.width = 1024, .height = 768}});
    app.app().runFrame();
    ASSERT_EQ(app.state().resizes.size(), 1U);
    EXPECT_EQ(app.state().resizes[0], glm::uvec2(1024, 768));
}

TEST(BackendPlugin, WaitsAndDrawsNothingWhileMinimized) {
    BackendApp app;
    app.state().framebufferSize = {0, 0};
    app.app().runFrame();
    EXPECT_EQ(app.state().waits, 1);
    EXPECT_EQ(app.state().polls, 0);
    EXPECT_EQ(app.state().beginFrames, 0);
    EXPECT_EQ(app.state().endFrames, 0);
}

TEST(BackendPlugin, TheAppStopsWhenTheWindowCloses) {
    BackendApp app;
    app.state().closeAfterPolls = 3;
    EXPECT_EQ(app.app().run(), 0);
    EXPECT_EQ(app.state().polls, 3);
}

TEST(BackendPlugin, DoesNotRunOnADedicatedServer) {
    BackendApp app{rtype::ecs::RunMode::kServer};
    app.app().runFrame();
    EXPECT_EQ(app.state().initCount, 0);
    EXPECT_EQ(app.state().polls, 0);
}

TEST(BackendPlugin, FeedsInputWhicheverPluginIsAddedFirst) {
    for (const bool inputFirst : {true, false}) {
        auto state = std::make_shared<rtype::test::plugins::FakeBackendState>();
        rtype::ecs::App app{rtype::ecs::RunMode::kClient};
        rtype::plugins::backend::BackendPlugin backend{rtype::test::plugins::makeFakeRegistry(state),
                                                       rtype::test::plugins::makeFakeSettings()};
        if (inputFirst) {
            app.addPlugin(rtype::plugins::input::InputPlugin{}).addPlugin(std::move(backend));
        } else {
            app.addPlugin(std::move(backend)).addPlugin(rtype::plugins::input::InputPlugin{});
        }
        state->frames.push_back({rtype::engine::event::KeyPressed{.key = rtype::engine::input::Key::kSpace}});
        app.runFrame();
        const auto& input = app.getWorld().getResources().require<rtype::plugins::input::Input>();
        EXPECT_GT(input.read(rtype::engine::input::Control::key(rtype::engine::input::Key::kSpace)), 0.0F)
            << (inputFirst ? "InputPlugin added first" : "BackendPlugin added first");
    }
}
