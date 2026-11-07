/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorPluginTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <string_view>
#include <variant>

#include "FakeBackend.hpp"
#include "editor/EditorPlugin.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/graphics/PlatformEventFilter.hpp"
#include "plugins/graphics/ToolFrame.hpp"
#include "plugins/graphics/WindowEvents.hpp"  // IWYU pragma: keep
#include "plugins/input/InputEvents.hpp"      // IWYU pragma: keep
#include "plugins/input/InputPlugin.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/contract/platform/event/Event.hpp"
#include "rtype/contract/platform/input/Key.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/exceptions/PluginException.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"

namespace {
namespace event = rtype::contract::platform::event;
using rtype::contract::platform::Event;
using rtype::contract::platform::input::Key;
using rtype::contract::platform::input::MouseButton;

/// @brief An App with the drawing plugins over the fake backend, and the editor, which keeps no layout file.
class EditorApp {
  public:
    EditorApp()
        : _state{std::make_shared<rtype::test::plugins::FakeBackendState>()}, _app{rtype::ecs::RunMode::kClient} {
        rtype::test::plugins::addDrawingPlugins(_app, rtype::test::plugins::makeFakeRegistry(_state),
                                                rtype::test::plugins::makeFakeSettings());
        _app.addPlugin(rtype::plugins::capsule::CapsulePlugin{});
        _app.addPlugin(rtype::editor::EditorPlugin{
            rtype::editor::EditorOptions{.layoutFile = std::filesystem::path{}, .captureOutput = false}});
    }
    ~EditorApp() = default;
    EditorApp(const EditorApp& other) = delete;
    EditorApp& operator=(const EditorApp& other) = delete;
    EditorApp(EditorApp&& other) = delete;
    EditorApp& operator=(EditorApp&& other) = delete;

    [[nodiscard]] rtype::ecs::App& app() noexcept { return _app; }
    [[nodiscard]] rtype::test::plugins::FakeBackendState& state() noexcept { return *_state; }

    /// @return What the window's filter says of @p received, which it may have changed.
    [[nodiscard]] bool filter(Event& received) {
        return _app.getWorld().getResources().getMut<rtype::plugins::graphics::PlatformEventFilter>()->filter(received);
    }

  private:
    std::shared_ptr<rtype::test::plugins::FakeBackendState> _state;  ///< Drives the fakes.
    rtype::ecs::App _app;                                            ///< The App under test.
};

constexpr int kLayoutFrames = 4;  ///< Frames the docking takes to give every panel its size.

/// @brief Runs @p count frames of @p editor.
void runFrames(EditorApp& editor, int count) {
    for (int frame = 0; frame < count; ++frame) {
        editor.app().runFrame();
    }
}
}  // namespace

TEST(EditorPlugin, IsNamedLikeTheLibraryEntryRegistersIt) {
    EXPECT_EQ(rtype::editor::EditorPlugin::kName, "rtype.Editor");
    EXPECT_EQ(rtype::editor::EditorPlugin{}.getName(), "rtype.Editor");
}

TEST(EditorPlugin, RequiresTheGraphicsAndCapsulePlugins) {
    const rtype::editor::EditorPlugin plugin{rtype::editor::EditorOptions{.layoutFile = std::filesystem::path{}}};
    const auto dependencies = plugin.getDependencies();
    ASSERT_EQ(dependencies.size(), 2U);
    EXPECT_EQ(dependencies[0], "rtype.graphics");
    EXPECT_EQ(dependencies[1], "rtype.Capsule");
}

TEST(EditorPlugin, IsRefusedWithoutTheGraphicsPlugin) {
    rtype::ecs::App app{rtype::ecs::RunMode::kClient};
    EXPECT_THROW(app.addPlugin(rtype::editor::EditorPlugin{
                     rtype::editor::EditorOptions{.layoutFile = std::filesystem::path{}, .captureOutput = false}}),
                 rtype::ecs::exceptions::PluginException);
}

TEST(EditorPlugin, SetsTheTwoHooksOfTheGraphicsPlugin) {
    EditorApp editor;
    const rtype::ecs::Resources& resources = editor.app().getWorld().getResources();
    EXPECT_TRUE(static_cast<bool>(resources.get<rtype::plugins::graphics::PlatformEventFilter>()->filter));
    EXPECT_TRUE(static_cast<bool>(resources.get<rtype::plugins::graphics::ToolFrame>()->draw));
}

TEST(EditorPlugin, HidesTheMouseFromTheGameOutsideItsPanel) {
    EditorApp editor;
    Event moved = event::MouseMoved{.position = {400.0F, 300.0F}};
    Event pressed = event::MouseButtonPressed{.button = MouseButton::kLeft};
    Event released = event::MouseButtonReleased{.button = MouseButton::kLeft};
    Event scrolled = event::MouseScrolled{.offset = {0.0F, 1.0F}};
    EXPECT_FALSE(editor.filter(moved));
    EXPECT_FALSE(editor.filter(pressed));
    EXPECT_FALSE(editor.filter(released));
    EXPECT_FALSE(editor.filter(scrolled));
}

TEST(EditorPlugin, HidesTheKeyboardFromTheGameWithoutTheFocusOnItsPanel) {
    EditorApp editor;
    Event pressed = event::KeyPressed{.key = Key::kA};
    Event typed = event::TextEntered{.codepoint = U'a'};
    EXPECT_FALSE(editor.filter(pressed));
    EXPECT_FALSE(editor.filter(typed));
}

TEST(EditorPlugin, AlwaysTellsTheGameAKeyWasReleased) {
    EditorApp editor;
    Event released = event::KeyReleased{.key = Key::kA};
    EXPECT_TRUE(editor.filter(released));
}

TEST(EditorPlugin, LetsTheWindowClosingAndFocusThrough) {
    EditorApp editor;
    Event closed = event::Closed{};
    Event focus = event::FocusChanged{.focused = true};
    Event pad = event::GamepadConnected{};
    EXPECT_TRUE(editor.filter(closed));
    EXPECT_TRUE(editor.filter(focus));
    EXPECT_TRUE(editor.filter(pad));
}

TEST(EditorPlugin, HidesTheWindowResizeFromTheGameUntilThereIsAScene) {
    EditorApp editor;
    Event resized = event::Resized{.width = 1024, .height = 768};
    EXPECT_FALSE(editor.filter(resized));
}

TEST(EditorPlugin, BuildsItsPanelsWhenTheFrameEnds) {
    EditorApp editor;
    editor.app().runFrame();
    ASSERT_EQ(editor.state().meshes.size(), 1U);
    const rtype::contract::renderer::Mesh& mesh = editor.state().meshes.front();
    EXPECT_FALSE(mesh.vertices.empty());
    EXPECT_FALSE(mesh.commands.empty());
    EXPECT_EQ(mesh.indices.size() % 3, 0U);
    EXPECT_TRUE(
        std::ranges::all_of(mesh.indices, [&mesh](std::uint32_t index) { return index < mesh.vertices.size(); }));
}

TEST(EditorPlugin, GivesTheRendererItsFontTexture) {
    EditorApp editor;
    editor.app().runFrame();
    EXPECT_GE(editor.state().createdTextures.size(), 1U);
}

TEST(EditorPlugin, StopsRemakingItsFontTextureOnceTheGlyphsAreKnown) {
    EditorApp editor;
    // The atlas grows as panels draw glyphs for the first time: each growth sends it whole, a few times at the start.
    for (int frame = 0; frame < 5; ++frame) {
        editor.app().runFrame();
    }
    const std::size_t created = editor.state().createdTextures.size();
    const std::size_t destroyed = editor.state().destroyedTextures.size();
    for (int frame = 0; frame < 5; ++frame) {
        editor.app().runFrame();
    }
    EXPECT_EQ(editor.state().createdTextures.size(), created);
    EXPECT_EQ(editor.state().destroyedTextures.size(), destroyed);
}

TEST(EditorPlugin, DestroysTheFontTextureItReplaces) {
    EditorApp editor;
    for (int frame = 0; frame < 5; ++frame) {
        editor.app().runFrame();
    }
    // Every texture made but the last one was replaced, and the renderer was told to free it.
    EXPECT_EQ(editor.state().destroyedTextures.size() + 1, editor.state().createdTextures.size());
}

TEST(EditorPlugin, DrawsEveryCommandInsideTheFramebuffer) {
    EditorApp editor;
    editor.app().runFrame();
    for (const auto& command : editor.state().meshes.front().commands) {
        EXPECT_GE(command.clipMin.x, 0.0F);
        EXPECT_GE(command.clipMin.y, 0.0F);
        EXPECT_LE(command.clipMax.x, 800.0F);
        EXPECT_LE(command.clipMax.y, 600.0F);
        EXPECT_LE(command.indexOffset + command.indexCount, editor.state().meshes.front().indices.size());
    }
}

TEST(EditorPlugin, AsksForTheSceneToBeDrawnAsBigAsItsGamePanel) {
    EditorApp editor;
    editor.state().sceneTexture = {.index = 1, .generation = 1};
    runFrames(editor, kLayoutFrames);
    ASSERT_FALSE(editor.state().sceneSizeRequests.empty());
    const auto& request = editor.state().sceneSizeRequests.back();
    ASSERT_TRUE(request.has_value());
    // A panel inside an 800x600 window: smaller than it, and not a sliver.
    EXPECT_GT(request->x, 100U);
    EXPECT_GT(request->y, 100U);
    EXPECT_LT(request->x, 800U);
    EXPECT_LT(request->y, 600U);
}

TEST(EditorPlugin, GivesTheGameTheMouseOverItsPanelInTheScenePixels) {
    EditorApp editor;
    editor.state().sceneTexture = {.index = 1, .generation = 1};
    runFrames(editor, kLayoutFrames);  // Lays the panels out and asks for the scene.
    // The scene is what the panel asked for, so the game sees one pixel for each pixel of the screen.
    editor.state().sceneSize = *editor.state().sceneSizeRequests.back();
    // The panel is under the mouse once a frame has seen the mouse there.
    editor.state().frames.push_back({event::MouseMoved{.position = {400.0F, 200.0F}}});
    editor.app().runFrame();
    editor.state().frames.push_back({event::MouseMoved{.position = {410.0F, 210.0F}, .delta = {10.0F, 10.0F}}});
    editor.app().runFrame();
    rtype::ecs::EventCursor cursor;
    const auto moves = editor.app().getWorld().getEvents().read<event::MouseMoved>(cursor);
    ASSERT_EQ(moves.size(), 1U);
    const glm::vec2 position = moves.begin()->position;
    const glm::uvec2 scene = *editor.state().sceneSize;
    EXPECT_GE(position.x, 0.0F);
    EXPECT_GE(position.y, 0.0F);
    EXPECT_LE(position.x, static_cast<float>(scene.x));
    EXPECT_LE(position.y, static_cast<float>(scene.y));
    EXPECT_EQ(moves.begin()->delta, glm::vec2(10.0F, 10.0F));
}

TEST(EditorPlugin, StartsEditingWithTheGameplayPaused) {
    EditorApp editor;
    EXPECT_TRUE(editor.app().getSchedules().isPaused());
}
