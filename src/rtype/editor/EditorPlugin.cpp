/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorPlugin
*/

#include "EditorPlugin.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <format>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <span>
#include <string>
#include <utility>

#include "EditorState.hpp"
#include "core/system/CacheDirectory.hpp"
#include "editor/EditorSession.hpp"
#include "editor/gui/GuiContext.hpp"
#include "editor/gui/GuiMesh.hpp"
#include "editor/panels/Panels.hpp"
#include "editor/play/OnlinePlay.hpp"
#include "editor/play/PlaySession.hpp"
#include "editor/scene/Isolation.hpp"
#include "editor/shell/TextEditor.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/PlatformEventFilter.hpp"
#include "plugins/graphics/ToolFrame.hpp"
#include "rtype/contract/platform/IPlatform.hpp"
#include "rtype/contract/platform/event/Event.hpp"
#include "rtype/contract/renderer/IRenderer.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/schedule/Schedules.hpp"

namespace rtype::editor {

namespace {
constexpr float kLeftShare = 0.2F;     ///< Share of the window the Hierarchy takes in the default layout.
constexpr float kRightShare = 0.25F;   ///< Share of what is left the Inspector takes.
constexpr float kBottomShare = 0.25F;  ///< Share of what is left the Assets and the Stats take.
constexpr int kLayoutVersion = 7;  ///< Raised when the panels change, so a kept layout is not read with the wrong ones.
constexpr float kFallbackFrame = 1.0F / 60.0F;  ///< The duration of the first frame, which has no previous one.

/// @return Where the layout of the panels is kept: the user's cache, as the editor is a tool and not a game's data.
std::filesystem::path layoutFilePath() {
    const std::filesystem::path cache = core::system::getCacheDirectory();
    // Versioned: a layout kept by an editor with other panels would leave the new ones floating.
    return cache.empty() ? std::filesystem::path{}
                         : cache / "rtype" / "editor" / std::format("layout-{}.ini", kLayoutVersion);
}

/// @return Whether a dock node holds nothing yet: no layout was read from the file.
bool isEmpty(const ImGuiDockNode* node) { return node == nullptr || (node->IsLeafNode() && node->Windows.Size == 0); }

/// @brief Docks the panels as by default: the Hierarchy on the left, the Inspector on the right, the Assets and the
/// Stats below, the game in the middle.
void buildDefaultLayout(ImGuiID dockspace) {
    ImGui::DockBuilderRemoveNode(dockspace);
    ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace, ImGui::GetMainViewport()->WorkSize);
    ImGuiID center = dockspace;
    const ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, kLeftShare, nullptr, &center);
    const ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, kRightShare, nullptr, &center);
    const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, kBottomShare, nullptr, &center);
    ImGui::DockBuilderDockWindow("Hierarchy", left);
    ImGui::DockBuilderDockWindow("Inspector", right);
    ImGui::DockBuilderDockWindow("Assets", bottom);
    ImGui::DockBuilderDockWindow("Stats", bottom);
    ImGui::DockBuilderDockWindow("Plugins", bottom);
    ImGui::DockBuilderDockWindow("Particles", bottom);
    ImGui::DockBuilderDockWindow("Console", bottom);
    ImGui::DockBuilderDockWindow("Server", bottom);
    ImGui::DockBuilderDockWindow("Game", center);
    ImGui::DockBuilderFinish(dockspace);
}

/// @brief Fills the dockspace over the window, and docks the panels as by default when there is no layout yet.
void drawDockspace(EditorState& state) {
    const ImGuiID dockspace = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
    const bool firstFrame = std::exchange(state.buildLayout, false);
    if (state.resetLayout || (firstFrame && isEmpty(ImGui::DockBuilderGetNode(dockspace)))) {
        state.resetLayout = false;
        buildDefaultLayout(dockspace);
    }
}

/// @brief Tells ImGui the size of the window, the density of its pixels and how long the last frame took, and scales
/// the panels to the desktop's content scale.
void beginFrameIo(EditorState& state, contract::platform::IPlatform& platform) {
    state.platform = &platform;
    const glm::uvec2 window = platform.getWindowSize();
    const glm::uvec2 framebuffer = platform.getFramebufferSize();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2{static_cast<float>(window.x), static_cast<float>(window.y)};
    io.DisplayFramebufferScale = ImVec2{static_cast<float>(framebuffer.x) / static_cast<float>(std::max(window.x, 1U)),
                                        static_cast<float>(framebuffer.y) / static_cast<float>(std::max(window.y, 1U))};
    // A scaled screen (200%) where the window is measured in pixels (X11, Windows): the panels are drawn bigger.
    state.context->setScale(gui::interfaceScale(platform.getContentScale().x, io.DisplayFramebufferScale.x));
    const double now = platform.getTime();
    io.DeltaTime = state.lastTime > 0.0 ? std::max(static_cast<float>(now - state.lastTime), 1.0e-5F) : kFallbackFrame;
    state.lastTime = now;
    // The system's clipboard, through the platform: ImGui copies and pastes with it, and "Copy as Luau" fills it.
    ImGuiPlatformIO& platformIo = ImGui::GetPlatformIO();
    platformIo.Platform_ClipboardUserData = &state;
    platformIo.Platform_SetClipboardTextFn = [](ImGuiContext* /*context*/, const char* text) {
        auto* editor = static_cast<EditorState*>(ImGui::GetPlatformIO().Platform_ClipboardUserData);
        editor->platform->setClipboardText(text);
    };
    platformIo.Platform_GetClipboardTextFn = [](ImGuiContext* /*context*/) -> const char* {
        auto* editor = static_cast<EditorState*>(ImGui::GetPlatformIO().Platform_ClipboardUserData);
        editor->clipboard = editor->platform->getClipboardText();
        return editor->clipboard.c_str();
    };
    state.frameTimes.at(state.frameCursor) = io.DeltaTime * 1000.0F;
    state.frameCursor = (state.frameCursor + 1) % state.frameTimes.size();
}

/// @brief The tool frame: builds the panels of the editor and hands their mesh to the renderer, over the window.
void runFrame(EditorState& state, contract::platform::IPlatform& platform, contract::renderer::IRenderer& renderer) {
    state.context->makeCurrent();
    if (!state.startScene.empty()) {  // the capsule files are read once every plugin is built: open it now
        const std::string start = std::exchange(state.startScene, {});
        if (std::filesystem::path{start}.extension() == ".scene") {
            panels::openScene(state, start);
        } else {
            panels::openPrefab(state, start);  // an actor opens alone, as a double-click in the Assets does
        }
    }
    beginFrameIo(state, platform);
    ImGui::NewFrame();
    panels::drawToolbar(state, platform);
    drawDockspace(state);
    panels::drawGamePanel(state, renderer);
    panels::drawPrefabPanel(state, renderer);
    panels::drawHierarchyPanel(state);
    panels::drawInspectorPanel(state);
    panels::drawAssetsPanel(state);
    panels::drawStatsPanel(state, renderer);
    panels::drawParticlesPanel(state);
    panels::drawConsolePanel(state);
    panels::drawServerPanel(state);
    panels::drawPluginsPanel(state);
    state.session->online.update(*state.supervisor);
    ImGui::Render();
    ImDrawData& drawData = *ImGui::GetDrawData();
    gui::updateTextures(drawData, renderer);
    renderer.drawMesh(gui::buildMesh(drawData));
}
}  // namespace

EditorPlugin::EditorPlugin(EditorOptions options) : _options{std::move(options)} {}

std::string_view EditorPlugin::getName() const noexcept { return kName; }

std::span<const std::string_view> EditorPlugin::getDependencies() const noexcept {
    static constexpr std::array<std::string_view, 2> kDependencies{plugins::graphics::GraphicsPlugin::kName,
                                                                   plugins::capsule::CapsulePlugin::kName};
    return kDependencies;
}

void EditorPlugin::build(ecs::App& app) {
    if (!_options.session) {
        _options.session = std::make_shared<EditorSession>(_options.captureOutput);
    }
    const auto state =
        std::make_shared<EditorState>(app, _options.layoutFile.value_or(layoutFilePath()), _options.session);
    play::OnlinePlay& online = state->session->online;
    if (online.isActive() && app.getSchedules().getRunMode() == ecs::RunMode::kClient) {
        // Play online: this App is the client of the server. The game runs at once, and the scene is not shown: the
        // world is the server's.
        online.adopt();
        state->play.play(app);
        state->status = "Online: playing as a client of the server";
    } else {
        if (online.isActive()) {  // the engine could not build a client and went back to its previous mode
            online.fail("The game could not be built as a client: see the Console");
        }
        // The editor starts editing: the gameplay waits for Play (Startup still runs, so the game sets itself up).
        play::PlaySession::edit(app);
        // Back from an online Play, the scene being edited is opened again.
        state->startScene =
            state->session->reopenScene.empty() ? _options.scene : std::move(state->session->reopenScene);
        state->session->reopenScene.clear();
    }
    state->textEditor = _options.textEditor.empty() ? shell::defaultTextEditor() : _options.textEditor;
    ecs::Resources& resources = app.getWorld().getResources();
    // The graphics plugin, which this one requires, inserted both hooks; they live and die with this App.
    resources.getMut<plugins::graphics::PlatformEventFilter>()->filter =
        [input = state->input](contract::platform::Event& event) { return input->filter(event); };
    resources.getMut<plugins::graphics::ToolFrame>()->draw = [state](contract::platform::IPlatform& platform,
                                                                     contract::renderer::IRenderer& renderer) {
        runFrame(*state, platform, renderer);
    };
}

}  // namespace rtype::editor
