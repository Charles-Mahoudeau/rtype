/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** BackendPlugin
*/

#include "BackendPlugin.hpp"

#include <algorithm>
#include <cstdint>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/BackendResources.hpp"
#include "plugins/backend/WindowEvents.hpp"  // IWYU pragma: keep
#include "plugins/backend/config/Settings.hpp"
#include "plugins/backend/event/Event.hpp"
#include "plugins/backend/event/PlatformEvent.hpp"
#include "plugins/backend/graphics/Camera.hpp"
#include "plugins/backend/graphics/Sprite.hpp"
#include "plugins/backend/graphics/SpriteRenderer.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/input/InputEvents.hpp"  // IWYU pragma: keep
#include "plugins/input/InputPlugin.hpp"
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

/// @brief Render: applies the camera of the entity with the highest priority; the first one found wins a tie.
void applyCamera(ecs::NonSendMut<plugins::backend::Backend> backend, ecs::Query<const graphics::Camera> cameras) {
    plugins::backend::Backend& started = *backend;
    if (!isStarted(started) || isMinimized(started)) {
        return;
    }
    const graphics::Camera* shown = nullptr;
    for (auto [entity, camera] : cameras) {
        if (shown == nullptr || camera.priority > shown->priority) {
            shown = &camera;
        }
    }
    if (shown != nullptr) {
        started.renderer->setCamera(*shown);
    }
}

/// @brief One sprite to draw, with the layer it is sorted by.
struct SpriteItem {
    std::int32_t layer;       ///< Draw order: higher on top.
    graphics::Sprite sprite;  ///< The draw command.
};

/// @brief Render: draws the sprites whose texture is loaded, by layer then by texture.
void drawSprites(ecs::NonSendMut<plugins::backend::Backend> backend, ecs::Res<TextureAssets> textures,
                 ecs::Query<const transform::GlobalTransform, const SpriteRenderer> sprites) {
    plugins::backend::Backend& started = *backend;
    if (!isStarted(started) || isMinimized(started)) {
        return;
    }
    std::vector<SpriteItem> items;
    for (auto [entity, global, sprite] : sprites) {
        const std::optional<graphics::TextureId> texture = textures->getTexture(sprite.texture);
        if (!texture) {
            continue;
        }
        items.push_back(SpriteItem{.layer = sprite.layer,
                                   .sprite = graphics::Sprite{.texture = *texture,
                                                              .source = sprite.source,
                                                              .position = global.position,
                                                              .scale = global.scale,
                                                              .origin = sprite.origin,
                                                              .rotation = global.rotation,
                                                              .color = sprite.color}});
    }
    std::ranges::stable_sort(items, [](const SpriteItem& left, const SpriteItem& right) {
        if (left.layer != right.layer) {
            return left.layer < right.layer;
        }
        return left.sprite.texture.index < right.sprite.texture.index;
    });
    for (const SpriteItem& item : items) {
        started.renderer->draw(item.sprite);
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
                             plugins::backend::config::Settings settings)
    : _registry{std::move(registry)}, _settings{std::move(settings)} {}

std::string_view BackendPlugin::getName() const noexcept { return kName; }

void BackendPlugin::build(ecs::App& app) {
    app.addPlugin(plugins::input::InputPlugin{});
    app.addPlugin(plugins::transform::TransformPlugin{});
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
