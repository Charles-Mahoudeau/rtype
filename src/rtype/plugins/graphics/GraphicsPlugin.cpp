/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GraphicsPlugin
*/

#include "GraphicsPlugin.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <glm/common.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "core/config/Settings.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/Camera.hpp"
#include "plugins/graphics/GraphicsResources.hpp"
#include "plugins/graphics/SpriteQueue.hpp"
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/graphics/VisibleArea.hpp"
#include "plugins/graphics/WindowEvents.hpp"  // IWYU pragma: keep
#include "plugins/input/InputEvents.hpp"      // IWYU pragma: keep
#include "plugins/input/InputPlugin.hpp"
#include "plugins/input/PlatformEvent.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/contract/platform/event/Event.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"
#include "rtype/contract/renderer/view/CameraView.hpp"
#include "rtype/contract/renderer/view/View.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/AppExit.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/Schedule.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"

namespace rtype::plugins::graphics {
namespace {
/// @return Whether the window shows nothing (minimized: a 0x0 framebuffer), so there is no frame to draw.
[[nodiscard]] bool isMinimized(const plugins::graphics::Backend& backend) {
    const glm::uvec2 size = backend.platform->getFramebufferSize();
    return size.x == 0 || size.y == 0;
}

/// @return Whether the Startup system created the backend.
[[nodiscard]] bool isStarted(const plugins::graphics::Backend& backend) noexcept {
    return backend.platform != nullptr && backend.renderer != nullptr;
}

/// @brief Startup: creates the platform and the renderer the configuration names.
void createBackend(ecs::NonSendMut<BackendSetup> setup, ecs::NonSendMut<plugins::graphics::Backend> backend) {
    *backend = setup->registry->createBackend(setup->settings);
}

/// @brief PreUpdate: sends every event of the platform as an ECS event, resizes the renderer, and asks the App to
/// stop once the window is closed.
void forwardEvents(ecs::NonSendMut<plugins::graphics::Backend> backend,
                   ecs::EventWriter<contract::platform::event::Closed> closed,
                   ecs::EventWriter<contract::platform::event::Resized> resized,
                   ecs::EventWriter<contract::platform::event::FocusChanged> focus,
                   ecs::EventWriter<contract::platform::event::KeyPressed> keyPressed,
                   ecs::EventWriter<contract::platform::event::KeyReleased> keyReleased,
                   ecs::EventWriter<contract::platform::event::TextEntered> text,
                   ecs::EventWriter<contract::platform::event::MouseMoved> mouseMoved,
                   ecs::EventWriter<contract::platform::event::MouseButtonPressed> buttonPressed,
                   ecs::EventWriter<contract::platform::event::MouseButtonReleased> buttonReleased,
                   ecs::EventWriter<contract::platform::event::MouseScrolled> scrolled,
                   ecs::EventWriter<contract::platform::event::GamepadConnected> gamepadConnected,
                   ecs::EventWriter<contract::platform::event::GamepadDisconnected> gamepadDisconnected,
                   ecs::EventWriter<contract::platform::event::GamepadButtonPressed> gamepadPressed,
                   ecs::EventWriter<contract::platform::event::GamepadButtonReleased> gamepadReleased,
                   ecs::EventWriter<contract::platform::event::GamepadAxisMoved> gamepadAxis,
                   ecs::EventWriter<plugins::input::PlatformEvent> platformEvents,
                   ecs::EventWriter<ecs::AppExit> exit) {
    plugins::graphics::Backend& started = *backend;
    if (!isStarted(started)) {
        return;
    }
    auto writers =
        std::tie(closed, resized, focus, keyPressed, keyReleased, text, mouseMoved, buttonPressed, buttonReleased,
                 scrolled, gamepadConnected, gamepadDisconnected, gamepadPressed, gamepadReleased, gamepadAxis);
    const std::vector<contract::platform::Event> events =
        isMinimized(started) ? started.platform->waitEvents() : started.platform->pollEvents();
    for (const contract::platform::Event& event : events) {
        platformEvents.send(plugins::input::PlatformEvent{.event = event});
        std::visit(
            [&writers, &started](const auto& alternative) {
                using Alternative = std::decay_t<decltype(alternative)>;
                if constexpr (std::is_same_v<Alternative, contract::platform::event::Resized>) {
                    started.renderer->resize({alternative.width, alternative.height});
                }
                std::get<ecs::EventWriter<Alternative>&>(writers).send(alternative);
            },
            event);
    }
    if (!started.platform->isOpen()) {
        exit.send(ecs::AppExit{.code = 0});
    }
}

/// @brief Render: destroys the unloaded textures and loads the queued ones, before the frame draws them.
void loadTextures(ecs::NonSendMut<plugins::graphics::Backend> backend, ecs::ResMut<TextureAssets> textures) {
    plugins::graphics::Backend& started = *backend;
    if (isStarted(started)) {
        updateTextures(*textures, *started.renderer);
    }
}

/// @brief Render: starts the frame, cleared with the ClearColor.
void beginFrame(ecs::NonSendMut<plugins::graphics::Backend> backend, ecs::Res<ClearColor> clearColor) {
    plugins::graphics::Backend& started = *backend;
    if (isStarted(started) && !isMinimized(started)) {
        started.renderer->beginFrame(clearColor->value);
    }
}

/// @return The box around what @p view shows in a @p framebufferSize framebuffer, for a renderer of @p viewSize.
graphics::VisibleArea visibleAreaOf(const contract::renderer::View& view, glm::vec2 viewSize,
                                    glm::uvec2 framebufferSize) {
    const contract::renderer::CameraView shown{view, viewSize, framebufferSize};
    const contract::renderer::Viewport& viewport = shown.getViewport();
    const std::array corners{viewport.position, viewport.position + glm::vec2{viewport.size.x, 0.0F},
                             viewport.position + glm::vec2{0.0F, viewport.size.y}, viewport.position + viewport.size};
    graphics::VisibleArea area{.min = shown.screenToWorld(corners.front()),
                               .max = shown.screenToWorld(corners.front())};
    for (const glm::vec2 corner : corners) {
        const glm::vec2 world = shown.screenToWorld(corner);
        area.min = glm::min(area.min, world);
        area.max = glm::max(area.max, world);
    }
    return area;
}

/// @brief Render: shows the world from the camera entity with the highest priority, at its GlobalTransform; the first
/// one found wins a tie. Records what the frame shows in the VisibleArea resource.
void applyCamera(ecs::NonSendMut<plugins::graphics::Backend> backend, ecs::ResMut<graphics::VisibleArea> visible,
                 ecs::Query<const transform::GlobalTransform, const graphics::Camera> cameras) {
    plugins::graphics::Backend& started = *backend;
    if (!isStarted(started) || isMinimized(started)) {
        *visible = graphics::VisibleArea{};
        return;
    }
    std::optional<contract::renderer::View> shown;
    std::int32_t shownPriority = 0;
    for (auto [entity, global, camera] : cameras) {
        if (!shown || camera.priority > shownPriority) {
            shown = contract::renderer::View{
                .position = global.position, .rotation = global.rotation, .projection = camera.projection};
            shownPriority = camera.priority;
        }
    }
    if (shown) {
        started.renderer->setView(*shown);
    }
    const glm::vec2 viewSize = started.renderer->getViewSize();
    const glm::uvec2 framebufferSize = started.platform->getFramebufferSize();
    *visible = visibleAreaOf(shown.value_or(contract::renderer::CameraView::makeDefaultView(viewSize, framebufferSize)),
                             viewSize, framebufferSize);
}

/// @brief Render: draws what the plugins pushed to the SpriteQueue, by layer then by texture, and empties it.
void drawSprites(ecs::NonSendMut<plugins::graphics::Backend> backend, ecs::ResMut<SpriteQueue> queue) {
    plugins::graphics::Backend& started = *backend;
    std::vector<SpriteQueue::Item> items = queue->take();
    if (!isStarted(started) || isMinimized(started)) {
        return;
    }
    std::ranges::stable_sort(items, [](const SpriteQueue::Item& left, const SpriteQueue::Item& right) {
        if (left.layer != right.layer) {
            return left.layer < right.layer;
        }
        return left.getTexture().index < right.getTexture().index;
    });
    for (const SpriteQueue::Item& item : items) {
        std::visit([&started](const auto& drawn) { started.renderer->draw(drawn); }, item.drawing);
    }
}

/// @brief Render: shows what was drawn since beginFrame().
void endFrame(ecs::NonSendMut<plugins::graphics::Backend> backend) {
    plugins::graphics::Backend& started = *backend;
    if (isStarted(started) && !isMinimized(started)) {
        started.renderer->endFrame();
    }
}

/// @brief Registers the event `T`, so it can be read before the first frame sends one.
template <typename T>
void registerEvent(ecs::Events& events) {
    static_cast<void>(events.registerType<T>());
}
}  // namespace

GraphicsPlugin::GraphicsPlugin(std::unique_ptr<plugins::graphics::BackendRegistry> registry,
                               core::config::Settings settings)
    : _registry{std::move(registry)}, _settings{std::move(settings)} {}

std::string_view GraphicsPlugin::getName() const noexcept { return kName; }

std::span<const std::string_view> GraphicsPlugin::getDependencies() const noexcept {
    static constexpr std::array<std::string_view, 1> kDependencies{input::InputPlugin::kName};
    return kDependencies;
}

void GraphicsPlugin::build(ecs::App& app) {
    app.addPlugin(plugins::transform::TransformPlugin{});
    ecs::Events& events = app.getWorld().getEvents();
    registerEvent<plugins::input::PlatformEvent>(events);
    registerEvent<contract::platform::event::Closed>(events);
    registerEvent<contract::platform::event::Resized>(events);
    registerEvent<contract::platform::event::TextEntered>(events);
    registerEvent<contract::platform::event::FocusChanged>(events);
    registerEvent<contract::platform::event::KeyPressed>(events);
    registerEvent<contract::platform::event::KeyReleased>(events);
    registerEvent<contract::platform::event::MouseMoved>(events);
    registerEvent<contract::platform::event::MouseButtonPressed>(events);
    registerEvent<contract::platform::event::MouseButtonReleased>(events);
    registerEvent<contract::platform::event::MouseScrolled>(events);
    registerEvent<contract::platform::event::GamepadConnected>(events);
    registerEvent<contract::platform::event::GamepadDisconnected>(events);
    registerEvent<contract::platform::event::GamepadButtonPressed>(events);
    registerEvent<contract::platform::event::GamepadButtonReleased>(events);
    registerEvent<contract::platform::event::GamepadAxisMoved>(events);

    ecs::Resources& resources = app.getWorld().getResources();
    resources.insertNonSend(BackendSetup{.registry = std::move(_registry), .settings = std::move(_settings)});
    resources.insertNonSend(plugins::graphics::Backend{});
    resources.insertIfAbsent<ClearColor>();
    resources.insertIfAbsent<TextureAssets>();
    resources.insertIfAbsent<graphics::SpriteQueue>();
    resources.insertIfAbsent<graphics::VisibleArea>();
    static_cast<void>(app.getWorld().getComponents().registerType<graphics::Camera>());

    app.addSystem(ecs::Phase::kStartup, std::string{kCreateSystem}, &createBackend).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kPreUpdate, std::string{kEventsSystem}, &forwardEvents)
        .before(std::string{plugins::input::InputPlugin::kUpdateSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kTexturesSystem}, &loadTextures).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kBeginFrameSystem}, &beginFrame)
        .after(std::string{kTexturesSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kCameraSystem}, &applyCamera)
        .after(std::string{kBeginFrameSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kSpritesSystem}, &drawSprites)
        .after(std::string{kCameraSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kEndFrameSystem}, &endFrame)
        .after(std::string{kSpritesSystem})
        .setSide(ecs::Side::kClient);
}
}  // namespace rtype::plugins::graphics
