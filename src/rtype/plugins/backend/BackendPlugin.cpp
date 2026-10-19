/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** BackendPlugin
*/

#include "BackendPlugin.hpp"

#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/BackendResources.hpp"
#include "plugins/backend/SpriteRenderer.hpp"
#include "plugins/backend/TextureAssets.hpp"
#include "plugins/backend/WindowEvents.hpp"
#include "plugins/backend/config/Settings.hpp"
#include "plugins/backend/event/Event.hpp"
#include "plugins/input/InputEvents.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/AppExit.hpp"
#include "rtype/ecs/event/Events.hpp"
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
                   ecs::EventWriter<ecs::AppExit> exit) {
    plugins::backend::Backend& started = *backend;
    if (!isStarted(started)) {
        return;
    }
    // One writer per alternative of plugins::backend::Event, found by the alternative's type.
    auto writers =
        std::tie(closed, resized, focus, keyPressed, keyReleased, text, mouseMoved, buttonPressed, buttonReleased,
                 scrolled, gamepadConnected, gamepadDisconnected, gamepadPressed, gamepadReleased, gamepadAxis);
    // Minimized: nothing to render, so sleep until an event (restore, close) arrives instead of spinning.
    const std::vector<plugins::backend::Event> events =
        isMinimized(started) ? started.platform->waitEvents() : started.platform->pollEvents();
    for (const plugins::backend::Event& event : events) {
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

/// @brief Render: starts the frame, cleared with the ClearColor.
void beginFrame(ecs::NonSendMut<plugins::backend::Backend> backend, ecs::Res<ClearColor> clearColor) {
    plugins::backend::Backend& started = *backend;
    if (isStarted(started) && !isMinimized(started)) {
        started.renderer->beginFrame(clearColor->value);
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
    ecs::Events& events = app.getWorld().getEvents();
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
    if (!resources.has<ClearColor>()) {  // a colour the game set before keeps
        resources.insert(ClearColor{});
    }
    if (!resources.has<TextureAssets>()) {  // textures the game recorded before keep
        resources.insert(TextureAssets{});
    }
    static_cast<void>(app.getWorld().getComponents().registerType<SpriteRenderer>());

    app.addSystem(ecs::Phase::kStartup, std::string{kCreateSystem}, &createBackend).setSide(ecs::Side::kClient);
    ecs::Schedule::SystemConfig forward =
        app.addSystem(ecs::Phase::kPreUpdate, std::string{kEventsSystem}, &forwardEvents).setSide(ecs::Side::kClient);
    // Input reads the events this system sends, in the same phase. A constraint naming a missing system is an error,
    // so it is set by whichever of the two plugins is added second.
    if (app.hasPlugin(plugins::input::InputPlugin::kName)) {
        forward.before(std::string{plugins::input::InputPlugin::kUpdateSystem});
    }
    app.addSystem(ecs::Phase::kRender, std::string{kBeginFrameSystem}, &beginFrame).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kEndFrameSystem}, &endFrame)
        .after(std::string{kBeginFrameSystem})
        .setSide(ecs::Side::kClient);
}
}  // namespace rtype::plugins::backend
