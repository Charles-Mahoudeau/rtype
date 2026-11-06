/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UiPluginTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "FakeBackend.hpp"
#include "plugins/ui/UiComponents.hpp"
#include "plugins/ui/UiEvents.hpp"
#include "plugins/ui/UiNode.hpp"
#include "plugins/ui/UiPlugin.hpp"
#include "plugins/ui/UiResources.hpp"
#include "plugins/ui/UiWidgets.hpp"
#include "rtype/contract/platform/event/Event.hpp"
#include "rtype/contract/platform/input/Key.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/world/World.hpp"

namespace event = rtype::contract::platform::event;
using rtype::contract::platform::input::GamepadButton;
using rtype::contract::platform::input::Key;
using rtype::contract::platform::input::MouseButton;
using rtype::ecs::Entity;
using rtype::plugins::ui::UiBackground;
using rtype::plugins::ui::UiButton;
using rtype::plugins::ui::UiClicked;
using rtype::plugins::ui::UiComputed;
using rtype::plugins::ui::UiFocus;
using rtype::plugins::ui::UiInteraction;
using rtype::plugins::ui::UiLength;
using rtype::plugins::ui::UiNode;
using rtype::plugins::ui::UiOverflow;
using rtype::plugins::ui::UiPlugin;
using rtype::plugins::ui::UiPointer;
using rtype::plugins::ui::UiPosition;
using rtype::plugins::ui::UiScroll;
using rtype::plugins::ui::UiSlider;
using rtype::plugins::ui::UiText;
using rtype::plugins::ui::UiTextChanged;
using rtype::plugins::ui::UiTextInput;
using rtype::plugins::ui::UiTextSubmitted;
using rtype::plugins::ui::UiToggle;
using rtype::plugins::ui::UiValueChanged;

namespace {
/// @return A node of 100 by 40 pixels at (@p x, @p y).
UiNode box(const float x, const float y) {
    return UiNode{.position = UiPosition::kAbsolute,
                  .width = UiLength::px(100.0F),
                  .height = UiLength::px(40.0F),
                  .left = UiLength::px(x),
                  .top = UiLength::px(y)};
}

/// @brief An App with the UiPlugin over the fake platform and renderer.
class UiApp {
  public:
    UiApp() : _state{std::make_shared<rtype::test::plugins::FakeBackendState>()}, _app{rtype::ecs::RunMode::kClient} {
        rtype::test::plugins::addDrawingPlugins(_app, rtype::test::plugins::makeFakeRegistry(_state),
                                                rtype::test::plugins::makeFakeSettings());
        _app.addPlugin(UiPlugin{});
    }
    ~UiApp() = default;
    UiApp(const UiApp& other) = delete;
    UiApp& operator=(const UiApp& other) = delete;
    UiApp(UiApp&& other) = delete;
    UiApp& operator=(UiApp&& other) = delete;

    /// @return A new entity with @p components.
    template <typename... Components>
    Entity spawn(Components... components) {
        rtype::ecs::Commands commands{_app.getWorld()};
        auto spawned = commands.spawn();
        (spawned.add(std::move(components)), ...);
        const Entity entity = spawned.getEntity();
        commands.flush();
        return entity;
    }

    /// @brief Makes @p child a child of @p parent.
    void parent(const Entity child, const Entity parent) { _app.getWorld().setParent(child, parent); }

    /// @brief Runs one frame, with @p events from the platform.
    void frame(std::vector<rtype::contract::platform::Event> events = {}) {
        _state->frames.push_back(std::move(events));
        _app.runFrame();
    }

    /// @brief Runs the frames that lay out what was spawned.
    void settle() {
        frame();
        frame();
    }

    /// @brief Clicks at @p position.
    void click(const glm::vec2 position) {
        frame({event::MouseMoved{.position = position}});
        frame({event::MouseButtonPressed{.button = MouseButton::kLeft}});
        frame({event::MouseButtonReleased{.button = MouseButton::kLeft}});
    }

    template <typename T>
    [[nodiscard]] const T& get(const Entity entity) {
        return *_app.getWorld().getComponents().get<T>(entity);
    }

    template <typename T>
    [[nodiscard]] const T& resource() {
        return _app.getWorld().getResources().require<T>();
    }

    template <typename T>
    [[nodiscard]] std::size_t count() {
        return _app.getWorld().getEvents().getCount<T>();
    }

    [[nodiscard]] rtype::test::plugins::FakeBackendState& state() noexcept { return *_state; }

  private:
    std::shared_ptr<rtype::test::plugins::FakeBackendState> _state;  ///< Drives the fake backend.
    rtype::ecs::App _app;                                            ///< The App.
};
}  // namespace

TEST(UiPlugin, LaysOutANodeIntoItsComputed) {
    UiApp app;
    const Entity node = app.spawn(box(10.0F, 20.0F));
    app.settle();
    EXPECT_FLOAT_EQ(app.get<UiComputed>(node).position.x, 10.0F);
    EXPECT_FLOAT_EQ(app.get<UiComputed>(node).size.y, 40.0F);
}

TEST(UiPlugin, DrawsABackgroundInTheOverlay) {
    UiApp app;
    static_cast<void>(
        app.spawn(box(0.0F, 0.0F), UiBackground{.color = {1.0F, 0.0F, 0.0F, 1.0F}, .cornerRadius = 4.0F}));
    app.settle();
    ASSERT_FALSE(app.state().shapes.empty());
    EXPECT_FLOAT_EQ(app.state().shapes.back().position.x, 50.0F);
    EXPECT_FLOAT_EQ(app.state().shapes.back().cornerRadius, 4.0F);
}

TEST(UiPlugin, AButtonSendsItsActionWhenClicked) {
    UiApp app;
    const Entity button = app.spawn(box(0.0F, 0.0F), UiBackground{}, UiInteraction{}, UiButton{.action = "menu.play"});
    app.settle();
    app.click({50.0F, 20.0F});
    EXPECT_EQ(app.resource<UiPointer>().hovered, button);
    EXPECT_EQ(app.count<UiClicked>(), 1U);
}

TEST(UiPlugin, ACoveredButtonIsNotClicked) {
    UiApp app;
    static_cast<void>(app.spawn(box(0.0F, 0.0F), UiInteraction{}, UiButton{}));
    UiNode cover = box(0.0F, 0.0F);
    cover.zIndex = 1;
    static_cast<void>(app.spawn(cover, UiBackground{}));
    app.settle();
    app.click({50.0F, 20.0F});
    EXPECT_TRUE(app.resource<UiPointer>().overUi);
    EXPECT_EQ(app.count<UiClicked>(), 0U);
}

TEST(UiPlugin, ADisabledButtonIgnoresThePointerAndTakesItsColour) {
    UiApp app;
    const UiButton style{.disabledColor = {0.1F, 0.2F, 0.3F, 1.0F}};
    const Entity button = app.spawn(box(0.0F, 0.0F), UiBackground{}, UiInteraction{.disabled = true}, style);
    app.settle();
    app.click({50.0F, 20.0F});
    EXPECT_EQ(app.count<UiClicked>(), 0U);
    EXPECT_FLOAT_EQ(app.get<UiBackground>(button).color.x, 0.1F);
}

TEST(UiPlugin, TheStateOfAButtonColoursIt) {
    UiApp app;
    const UiButton style{.hoveredColor = {0.5F, 0.0F, 0.0F, 1.0F}};
    const Entity button = app.spawn(box(0.0F, 0.0F), UiBackground{}, UiInteraction{}, style);
    app.settle();
    app.frame({event::MouseMoved{.position = {50.0F, 20.0F}}});
    EXPECT_TRUE(app.get<UiInteraction>(button).hovered);
    EXPECT_FLOAT_EQ(app.get<UiBackground>(button).color.x, 0.5F);
}

TEST(UiPlugin, AToggleFlipsAndSendsItsValue) {
    UiApp app;
    const Entity toggle = app.spawn(box(0.0F, 0.0F), UiBackground{}, UiInteraction{}, UiToggle{.action = "fullscreen"});
    app.settle();
    app.click({50.0F, 20.0F});
    EXPECT_TRUE(app.get<UiToggle>(toggle).checked);
    EXPECT_EQ(app.count<UiValueChanged>(), 1U);
}

TEST(UiPlugin, ASliderFollowsThePointerAndItsStep) {
    UiApp app;
    const Entity slider = app.spawn(box(0.0F, 0.0F), UiInteraction{}, UiSlider{.max = 10.0F, .step = 1.0F});
    app.settle();
    app.frame({event::MouseMoved{.position = {52.0F, 20.0F}}});
    app.frame({event::MouseButtonPressed{.button = MouseButton::kLeft}});
    app.frame();
    EXPECT_FLOAT_EQ(app.get<UiSlider>(slider).value, 5.0F);
}

TEST(UiPlugin, TabMovesTheFocusAndEnterActivatesIt) {
    UiApp app;
    const Entity first = app.spawn(box(0.0F, 0.0F), UiInteraction{}, UiButton{.action = "first"});
    const Entity second = app.spawn(box(0.0F, 50.0F), UiInteraction{}, UiButton{.action = "second"});
    app.settle();
    app.frame({event::KeyPressed{.key = Key::kTab}});
    EXPECT_EQ(app.resource<UiFocus>().focused, first);
    app.frame({event::KeyPressed{.key = Key::kTab}});
    EXPECT_EQ(app.resource<UiFocus>().focused, second);
    app.frame({event::KeyPressed{.key = Key::kEnter}});
    EXPECT_EQ(app.count<UiClicked>(), 1U);
}

TEST(UiPlugin, TheGamepadMovesTheFocusAndAdjustsASlider) {
    UiApp app;
    const Entity first = app.spawn(box(0.0F, 0.0F), UiInteraction{}, UiButton{});
    const Entity slider = app.spawn(box(0.0F, 50.0F), UiInteraction{}, UiSlider{.max = 10.0F, .step = 1.0F});
    app.settle();
    app.frame({event::GamepadButtonPressed{.button = GamepadButton::kDpadDown}});
    EXPECT_EQ(app.resource<UiFocus>().focused, first);
    app.frame({event::GamepadButtonPressed{.button = GamepadButton::kDpadDown}});
    EXPECT_EQ(app.resource<UiFocus>().focused, slider);
    app.frame({event::GamepadButtonPressed{.button = GamepadButton::kDpadRight}});
    EXPECT_FLOAT_EQ(app.get<UiSlider>(slider).value, 1.0F);
}

TEST(UiPlugin, ATextInputTakesWhatIsTypedAndKnowsWhereTheCaretIs) {
    UiApp app;
    const Entity input = app.spawn(box(0.0F, 0.0F), UiText{}, UiInteraction{}, UiTextInput{.action = "name"});
    app.settle();
    app.click({50.0F, 20.0F});
    EXPECT_TRUE(app.resource<UiFocus>().typing);
    app.frame({event::TextEntered{.codepoint = U'a'}, event::TextEntered{.codepoint = U'\u00e9'}});
    EXPECT_EQ(app.get<UiText>(input).text, "a\xC3\xA9");
    EXPECT_EQ(app.get<UiTextInput>(input).caret, 3U);
    app.frame({event::KeyPressed{.key = Key::kBackspace}});
    EXPECT_EQ(app.get<UiText>(input).text, "a");
    EXPECT_GE(app.count<UiTextChanged>(), 1U);
    app.frame({event::KeyPressed{.key = Key::kEnter}});
    EXPECT_EQ(app.count<UiTextSubmitted>(), 1U);
}

TEST(UiPlugin, ATextInputRespectsItsMaxLength) {
    UiApp app;
    const Entity input = app.spawn(box(0.0F, 0.0F), UiText{}, UiInteraction{}, UiTextInput{.maxLength = 1});
    app.settle();
    app.click({50.0F, 20.0F});
    app.frame({event::TextEntered{.codepoint = U'a'}, event::TextEntered{.codepoint = U'b'}});
    EXPECT_EQ(app.get<UiText>(input).text, "a");
}

TEST(UiPlugin, TheWheelScrollsInsideTheContent) {
    UiApp app;
    UiNode list = box(0.0F, 0.0F);
    list.overflow = UiOverflow::kScroll;
    const Entity scroll = app.spawn(list, UiScroll{.speed = 10.0F});
    const Entity content = app.spawn(UiNode{.height = UiLength::px(100.0F)});
    app.parent(content, scroll);
    app.settle();
    app.frame({event::MouseMoved{.position = {50.0F, 20.0F}}});
    app.frame({event::MouseScrolled{.offset = {0.0F, -3.0F}}});
    EXPECT_FLOAT_EQ(app.get<UiScroll>(scroll).offset.y, 30.0F);
    app.frame({event::MouseScrolled{.offset = {0.0F, -30.0F}}});
    EXPECT_FLOAT_EQ(app.get<UiScroll>(scroll).offset.y, 60.0F);  // 100 of content in a node of 40.
    app.frame();
    EXPECT_FLOAT_EQ(app.get<UiComputed>(content).position.y, -60.0F);
    EXPECT_FLOAT_EQ(app.get<UiComputed>(content).clip.w, 40.0F);
}

TEST(UiPlugin, AClickOutsideTakesTheFocusAway) {
    UiApp app;
    const Entity button = app.spawn(box(0.0F, 0.0F), UiInteraction{}, UiButton{});
    app.settle();
    app.click({50.0F, 20.0F});
    EXPECT_EQ(app.resource<UiFocus>().focused, button);
    app.frame({event::KeyPressed{.key = Key::kEscape}});
    EXPECT_TRUE(app.resource<UiFocus>().focused.isNull());
}
