/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** BackendPlugin
*/

#include "BackendPlugin.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <glm/common.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "core/config/Settings.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/BackendResources.hpp"
#include "plugins/backend/WindowEvents.hpp"  // IWYU pragma: keep
#include "plugins/backend/event/Event.hpp"
#include "plugins/backend/event/PlatformEvent.hpp"
#include "plugins/backend/graphics/Camera.hpp"
#include "plugins/backend/graphics/CameraView.hpp"
#include "plugins/backend/graphics/Sprite.hpp"
#include "plugins/backend/graphics/SpriteQueue.hpp"
#include "plugins/backend/graphics/SpriteRenderer.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/backend/graphics/View.hpp"
#include "plugins/backend/graphics/VisibleArea.hpp"
#include "plugins/input/InputEvents.hpp"  // IWYU pragma: keep
#include "plugins/input/InputPlugin.hpp"
#include "plugins/particle/ParticlePlugin.hpp"
#include "plugins/sprite/SpritePlugin.hpp"
#include "plugins/text/TextPlugin.hpp"
#include "plugins/tilemap/TilemapPlugin.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
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

namespace rtype::plugins::backend {
namespace {
/// @return Whether the window shows nothing (minimized: a 0x0 framebuffer), so there is no frame to draw.
[[nodiscard]] bool isMinimized(const plugins::backend::Backend& backend) {
    const glm::uvec2 size = backend.platform->getFramebufferSize();
    return size.x == 0 || size.y == 0;
}

/// @return Whether the Startup system created the backend.
[[nodiscard]] bool isStarted(const plugins::backend::Backend& backend) noexcept {
    return backend.platform != nullptr && backend.renderer != nullptr;
}

/// @brief Startup: creates the platform and the renderer the configuration names.
void createBackend(ecs::NonSendMut<BackendSetup> setup, ecs::NonSendMut<plugins::backend::Backend> backend) {
    *backend = setup->registry->createBackend(setup->settings);
}

/// @brief PreUpdate: sends every event of the platform as an ECS event, resizes the renderer, and asks the App to
/// stop once the window is closed.
void forwardEvents(ecs::NonSendMut<plugins::backend::Backend> backend,
                   ecs::EventWriter<plugins::backend::event::Closed> closed,
                   ecs::EventWriter<plugins::backend::event::Resized> resized,
                   ecs::EventWriter<plugins::backend::event::FocusChanged> focus,
                   ecs::EventWriter<plugins::backend::event::KeyPressed> keyPressed,
                   ecs::EventWriter<plugins::backend::event::KeyReleased> keyReleased,
                   ecs::EventWriter<plugins::backend::event::TextEntered> text,
                   ecs::EventWriter<plugins::backend::event::MouseMoved> mouseMoved,
                   ecs::EventWriter<plugins::backend::event::MouseButtonPressed> buttonPressed,
                   ecs::EventWriter<plugins::backend::event::MouseButtonReleased> buttonReleased,
                   ecs::EventWriter<plugins::backend::event::MouseScrolled> scrolled,
                   ecs::EventWriter<plugins::backend::event::GamepadConnected> gamepadConnected,
                   ecs::EventWriter<plugins::backend::event::GamepadDisconnected> gamepadDisconnected,
                   ecs::EventWriter<plugins::backend::event::GamepadButtonPressed> gamepadPressed,
                   ecs::EventWriter<plugins::backend::event::GamepadButtonReleased> gamepadReleased,
                   ecs::EventWriter<plugins::backend::event::GamepadAxisMoved> gamepadAxis,
                   ecs::EventWriter<plugins::backend::event::PlatformEvent> platformEvents,
                   ecs::EventWriter<ecs::AppExit> exit) {
    plugins::backend::Backend& started = *backend;
    if (!isStarted(started)) {
        return;
    }
    auto writers =
        std::tie(closed, resized, focus, keyPressed, keyReleased, text, mouseMoved, buttonPressed, buttonReleased,
                 scrolled, gamepadConnected, gamepadDisconnected, gamepadPressed, gamepadReleased, gamepadAxis);
    const std::vector<plugins::backend::Event> events =
        isMinimized(started) ? started.platform->waitEvents() : started.platform->pollEvents();
    for (const plugins::backend::Event& event : events) {
        platformEvents.send(plugins::backend::event::PlatformEvent{.event = event});
        std::visit(
            [&writers, &started](const auto& alternative) {
                using Alternative = std::decay_t<decltype(alternative)>;
                if constexpr (std::is_same_v<Alternative, plugins::backend::event::Resized>) {
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
void loadTextures(ecs::NonSendMut<plugins::backend::Backend> backend, ecs::ResMut<TextureAssets> textures) {
    plugins::backend::Backend& started = *backend;
    if (isStarted(started)) {
        updateTextures(*textures, *started.renderer);
    }
}

/// @brief Render: starts the frame, cleared with the ClearColor.
void beginFrame(ecs::NonSendMut<plugins::backend::Backend> backend, ecs::Res<ClearColor> clearColor) {
    plugins::backend::Backend& started = *backend;
    if (isStarted(started) && !isMinimized(started)) {
        started.renderer->beginFrame(clearColor->value);
    }
}

/// @return The box around what @p view shows in a @p framebufferSize framebuffer, for a renderer of @p viewSize.
graphics::VisibleArea visibleAreaOf(const graphics::View& view, glm::vec2 viewSize, glm::uvec2 framebufferSize) {
    const graphics::CameraView shown{view, viewSize, framebufferSize};
    const graphics::Viewport& viewport = shown.getViewport();
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
void applyCamera(ecs::NonSendMut<plugins::backend::Backend> backend, ecs::ResMut<graphics::VisibleArea> visible,
                 ecs::Query<const transform::GlobalTransform, const graphics::Camera> cameras) {
    plugins::backend::Backend& started = *backend;
    if (!isStarted(started) || isMinimized(started)) {
        *visible = graphics::VisibleArea{};
        return;
    }
    std::optional<graphics::View> shown;
    std::int32_t shownPriority = 0;
    for (auto [entity, global, camera] : cameras) {
        if (!shown || camera.priority > shownPriority) {
            shown = graphics::View{
                .position = global.position, .rotation = global.rotation, .projection = camera.projection};
            shownPriority = camera.priority;
        }
    }
    if (shown) {
        started.renderer->setView(*shown);
    }
    const glm::vec2 viewSize = started.renderer->getViewSize();
    const glm::uvec2 framebufferSize = started.platform->getFramebufferSize();
    *visible = visibleAreaOf(shown.value_or(graphics::CameraView::makeDefaultView(viewSize, framebufferSize)), viewSize,
                             framebufferSize);
}

/// @brief Render: draws the sprites whose texture is loaded and the queued sprites and texts (SpriteQueue), by layer
/// then by texture.
void drawSprites(ecs::NonSendMut<plugins::backend::Backend> backend, ecs::Res<TextureAssets> textures,
                 ecs::ResMut<graphics::SpriteQueue> queue,
                 ecs::Query<const transform::GlobalTransform, const SpriteRenderer> sprites) {
    plugins::backend::Backend& started = *backend;
    std::vector<graphics::SpriteQueue::Item> queued = queue->take();
    if (!isStarted(started) || isMinimized(started)) {
        return;
    }
    std::vector<graphics::SpriteQueue::Item> items;
    for (auto [entity, global, sprite] : sprites) {
        const std::optional<graphics::TextureId> texture = textures->getTexture(sprite.texture);
        if (!texture) {
            continue;
        }
        items.push_back(graphics::SpriteQueue::Item{.layer = sprite.layer,
                                                    .drawing = graphics::Sprite{.texture = *texture,
                                                                                .source = sprite.source,
                                                                                .position = global.position,
                                                                                .scale = global.scale,
                                                                                .origin = sprite.origin,
                                                                                .rotation = global.rotation,
                                                                                .color = sprite.color,
                                                                                .blend = sprite.blend}});
    }
    items.insert(items.end(), std::make_move_iterator(queued.begin()), std::make_move_iterator(queued.end()));
    std::ranges::stable_sort(items,
                             [](const graphics::SpriteQueue::Item& left, const graphics::SpriteQueue::Item& right) {
                                 if (left.layer != right.layer) {
                                     return left.layer < right.layer;
                                 }
                                 return left.getTexture().index < right.getTexture().index;
                             });
    for (const graphics::SpriteQueue::Item& item : items) {
        std::visit([&started](const auto& drawn) { started.renderer->draw(drawn); }, item.drawing);
    }
}

/// @brief Render: shows what was drawn since beginFrame().
void endFrame(ecs::NonSendMut<plugins::backend::Backend> backend) {
    plugins::backend::Backend& started = *backend;
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

BackendPlugin::BackendPlugin(std::unique_ptr<plugins::backend::BackendRegistry> registry,
                             core::config::Settings settings)
    : _registry{std::move(registry)}, _settings{std::move(settings)} {}

std::string_view BackendPlugin::getName() const noexcept { return kName; }

void BackendPlugin::build(ecs::App& app) {
    app.addPlugin(plugins::input::InputPlugin{});
    app.addPlugin(plugins::transform::TransformPlugin{});
    app.addPlugin(plugins::sprite::SpritePlugin{});
    app.addPlugin(plugins::tilemap::TilemapPlugin{});
    app.addPlugin(plugins::text::TextPlugin{});
    app.addPlugin(plugins::particle::ParticlePlugin{});
    ecs::Events& events = app.getWorld().getEvents();
    registerEvent<plugins::backend::event::PlatformEvent>(events);
    registerEvent<plugins::backend::event::Closed>(events);
    registerEvent<plugins::backend::event::Resized>(events);
    registerEvent<plugins::backend::event::TextEntered>(events);
    registerEvent<plugins::backend::event::FocusChanged>(events);
    registerEvent<plugins::backend::event::KeyPressed>(events);
    registerEvent<plugins::backend::event::KeyReleased>(events);
    registerEvent<plugins::backend::event::MouseMoved>(events);
    registerEvent<plugins::backend::event::MouseButtonPressed>(events);
    registerEvent<plugins::backend::event::MouseButtonReleased>(events);
    registerEvent<plugins::backend::event::MouseScrolled>(events);
    registerEvent<plugins::backend::event::GamepadConnected>(events);
    registerEvent<plugins::backend::event::GamepadDisconnected>(events);
    registerEvent<plugins::backend::event::GamepadButtonPressed>(events);
    registerEvent<plugins::backend::event::GamepadButtonReleased>(events);
    registerEvent<plugins::backend::event::GamepadAxisMoved>(events);

    ecs::Resources& resources = app.getWorld().getResources();
    resources.insertNonSend(BackendSetup{.registry = std::move(_registry), .settings = std::move(_settings)});
    resources.insertNonSend(plugins::backend::Backend{});
    if (!resources.has<ClearColor>()) {
        resources.insert(ClearColor{});
    }
    if (!resources.has<TextureAssets>()) {
        resources.insert(TextureAssets{});
    }
    if (!resources.has<graphics::SpriteQueue>()) {
        resources.insert(graphics::SpriteQueue{});
    }
    if (!resources.has<graphics::VisibleArea>()) {
        resources.insert(graphics::VisibleArea{});
    }
    static_cast<void>(app.getWorld().getComponents().registerType<SpriteRenderer>());
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
}  // namespace rtype::plugins::backend
