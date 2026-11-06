/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputPluginTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float2.hpp>

#include "plugins/input/Control.hpp"
#include "plugins/input/Input.hpp"
#include "plugins/input/InputAction.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/input/PlatformEvent.hpp"
#include "rtype/contract/platform/event/Event.hpp"
#include "rtype/contract/platform/input/Key.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"

static_assert(rtype::ecs::Resource<rtype::plugins::input::Input>);
static_assert(rtype::ecs::Event<rtype::plugins::input::PlatformEvent>);

namespace {
/// @brief An App with the InputPlugin and a "fire" action bound to Space.
class InputApp {
  public:
    explicit InputApp(rtype::ecs::RunMode mode = rtype::ecs::RunMode::kClient) : _app{mode} {
        _app.addPlugin(rtype::plugins::input::InputPlugin{});
        input()
            .addAction("fire", rtype::plugins::input::ActionType::kButton)
            .bind(rtype::plugins::input::Control::key(rtype::contract::platform::input::Key::kSpace));
    }
    ~InputApp() = default;
    InputApp(const InputApp& other) = delete;
    InputApp& operator=(const InputApp& other) = delete;
    InputApp(InputApp&& other) = delete;
    InputApp& operator=(InputApp&& other) = delete;

    [[nodiscard]] rtype::ecs::App& app() noexcept { return _app; }

    [[nodiscard]] rtype::plugins::input::Input& input() {
        return *_app.getWorld().getResources().requireMut<rtype::plugins::input::Input>();
    }

    /// @brief Sends an event, as the window plugin does after polling the platform.
    template <typename T>
    void send(T event) {
        _app.getWorld().getEvents().send(rtype::plugins::input::PlatformEvent{.event = event});
    }

    [[nodiscard]] const rtype::plugins::input::InputAction& fire() { return input().getAction("fire"); }

  private:
    rtype::ecs::App _app;  ///< The App under test.
};

const rtype::contract::platform::event::KeyPressed kSpacePressed{.key = rtype::contract::platform::input::Key::kSpace};
const rtype::contract::platform::event::KeyReleased kSpaceReleased{.key =
                                                                       rtype::contract::platform::input::Key::kSpace};
}  // namespace

TEST(InputPlugin, InsertsInputAndRegistersTheEvents) {
    rtype::ecs::App app;
    app.addPlugin(rtype::plugins::input::InputPlugin{});
    EXPECT_TRUE(app.hasPlugin("rtype.Input"));
    EXPECT_TRUE(app.getWorld().getResources().has<rtype::plugins::input::Input>());
    EXPECT_TRUE(app.getWorld().getEvents().find("rtype.PlatformEvent"));
}

TEST(InputPlugin, AKeyPressedThisFrameIsPressedThenHeld) {
    InputApp app;
    app.send(kSpacePressed);
    app.app().runFrame();
    EXPECT_TRUE(app.fire().isPressed());
    EXPECT_TRUE(app.fire().isHeld());
    app.app().runFrame();
    EXPECT_FALSE(app.fire().isPressed());
    EXPECT_TRUE(app.fire().isHeld());
}

TEST(InputPlugin, AKeyReleasedIsReleasedTheNextFrame) {
    InputApp app;
    app.send(kSpacePressed);
    app.app().runFrame();
    app.send(kSpaceReleased);
    app.app().runFrame();
    EXPECT_FALSE(app.fire().isHeld());
    EXPECT_TRUE(app.fire().isReleased());
}

TEST(InputPlugin, ATapWithinOneFrameStillCounts) {
    InputApp app;
    app.send(kSpacePressed);
    app.send(kSpaceReleased);
    app.app().runFrame();
    EXPECT_TRUE(app.fire().isPressed());
}

TEST(InputPlugin, AReleaseThenPressWithinOneFrameReadsAsHeld) {
    InputApp app;
    app.send(kSpacePressed);
    app.app().runFrame();
    app.send(kSpaceReleased);
    app.send(kSpacePressed);
    app.app().runFrame();
    EXPECT_TRUE(app.fire().isHeld());
}

TEST(InputPlugin, AnEventIsFedOnce) {
    InputApp app;
    app.send(kSpacePressed);
    app.app().runFrame();
    app.send(kSpaceReleased);
    app.app().runFrame();
    app.app().runFrame();
    EXPECT_FALSE(app.fire().isHeld());
}

TEST(InputPlugin, LosingTheFocusReleasesTheKeys) {
    InputApp app;
    app.send(kSpacePressed);
    app.app().runFrame();
    app.send(rtype::contract::platform::event::FocusChanged{.focused = false});
    app.app().runFrame();
    EXPECT_FALSE(app.fire().isHeld());
}

TEST(InputPlugin, TracksTheMouse) {
    InputApp app;
    app.send(rtype::contract::platform::event::MouseMoved{.position = {12.0F, 34.0F}, .delta = {2.0F, 4.0F}});
    app.app().runFrame();
    EXPECT_EQ(app.input().getMousePosition(), glm::vec2(12.0F, 34.0F));
}

TEST(InputPlugin, CountsEventsWithoutData) {
    InputApp app;
    app.send(rtype::contract::platform::event::GamepadConnected{});
    app.app().runFrame();
    EXPECT_TRUE(app.input().isGamepadConnected());
    app.send(rtype::contract::platform::event::GamepadDisconnected{});
    app.app().runFrame();
    EXPECT_FALSE(app.input().isGamepadConnected());
}

TEST(InputPlugin, DoesNotRunOnADedicatedServer) {
    InputApp app{rtype::ecs::RunMode::kServer};
    app.send(kSpacePressed);
    app.app().runFrame();
    EXPECT_FALSE(app.fire().isHeld());
}

TEST(InputPlugin, IsAddedOnce) {
    rtype::ecs::App app;
    app.addPlugin(rtype::plugins::input::InputPlugin{});
    app.getWorld().getResources().requireMut<rtype::plugins::input::Input>()->addAction(
        "fire", rtype::plugins::input::ActionType::kButton);
    app.addPlugin(rtype::plugins::input::InputPlugin{});
    EXPECT_NO_THROW(
        static_cast<void>(app.getWorld().getResources().require<rtype::plugins::input::Input>().getAction("fire")));
}
