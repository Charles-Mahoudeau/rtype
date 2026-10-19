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

#include "engine/backend/BackendRegistry.hpp"
#include "engine/config/Settings.hpp"
#include "engine/event/Event.hpp"
#include "plugins/backend/BackendResources.hpp"
#include "plugins/backend/WindowEvents.hpp"
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
[[nodiscard]] bool isMinimized(const engine::backend::Backend& backend) {
    const glm::uvec2 size = backend.platform->getFramebufferSize();
    return size.x == 0 || size.y == 0;
}

/// @return Whether the Startup system created the backend.
[[nodiscard]] bool isStarted(const engine::backend::Backend& backend) noexcept {
    return backend.platform != nullptr && backend.renderer != nullptr;
}

/// @brief Startup: creates the platform and the renderer the configuration names.
void createBackend(ecs::NonSendMut<BackendSetup> setup, ecs::NonSendMut<engine::backend::Backend> backend) {
    *backend = setup->registry->createBackend(setup->settings);
}

/// @brief PreUpdate: sends every event of the platform as an ECS event, resizes the renderer, and asks the App to
/// stop once the window is closed.
void forwardEvents(ecs::NonSendMut<engine::backend::Backend> backend, ecs::EventWriter<engine::event::Closed> closed,
                   ecs::EventWriter<engine::event::Resized> resized,
                   ecs::EventWriter<engine::event::FocusChanged> focus,
                   ecs::EventWriter<engine::event::KeyPressed> keyPressed,
                   ecs::EventWriter<engine::event::KeyReleased> keyReleased,
                   ecs::EventWriter<engine::event::TextEntered> text,
                   ecs::EventWriter<engine::event::MouseMoved> mouseMoved,
                   ecs::EventWriter<engine::event::MouseButtonPressed> buttonPressed,
                   ecs::EventWriter<engine::event::MouseButtonReleased> buttonReleased,
                   ecs::EventWriter<engine::event::MouseScrolled> scrolled,
                   ecs::EventWriter<engine::event::GamepadConnected> gamepadConnected,
                   ecs::EventWriter<engine::event::GamepadDisconnected> gamepadDisconnected,
                   ecs::EventWriter<engine::event::GamepadButtonPressed> gamepadPressed,
                   ecs::EventWriter<engine::event::GamepadButtonReleased> gamepadReleased,
                   ecs::EventWriter<engine::event::GamepadAxisMoved> gamepadAxis, ecs::EventWriter<ecs::AppExit> exit) {
    engine::backend::Backend& started = *backend;
    if (!isStarted(started)) {
        return;
    }
    // One writer per alternative of engine::Event, found by the alternative's type.
    auto writers =
        std::tie(closed, resized, focus, keyPressed, keyReleased, text, mouseMoved, buttonPressed, buttonReleased,
                 scrolled, gamepadConnected, gamepadDisconnected, gamepadPressed, gamepadReleased, gamepadAxis);
    // Minimized: nothing to render, so sleep until an event (restore, close) arrives instead of spinning.
    const std::vector<engine::Event> events =
        isMinimized(started) ? started.platform->waitEvents() : started.platform->pollEvents();
    for (const engine::Event& event : events) {
        std::visit(
            [&writers, &started](const auto& alternative) {
                using Alternative = std::decay_t<decltype(alternative)>;
                if constexpr (std::is_same_v<Alternative, engine::event::Resized>) {
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
void beginFrame(ecs::NonSendMut<engine::backend::Backend> backend, ecs::Res<ClearColor> clearColor) {
    engine::backend::Backend& started = *backend;
    if (isStarted(started) && !isMinimized(started)) {
        started.renderer->beginFrame(clearColor->value);
    }
}

/// @brief Render: shows what was drawn since beginFrame().
void endFrame(ecs::NonSendMut<engine::backend::Backend> backend) {
    engine::backend::Backend& started = *backend;
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

BackendPlugin::BackendPlugin(std::unique_ptr<engine::backend::BackendRegistry> registry,
                             engine::config::Settings settings)
    : _registry{std::move(registry)}, _settings{std::move(settings)} {}

std::string_view BackendPlugin::getName() const noexcept { return kName; }

void BackendPlugin::build(ecs::App& app) {
    ecs::Events& events = app.getWorld().getEvents();
    registerEvent<engine::event::Closed>(events);
    registerEvent<engine::event::Resized>(events);
    registerEvent<engine::event::TextEntered>(events);
    registerEvent<engine::event::FocusChanged>(events);
    registerEvent<engine::event::KeyPressed>(events);
    registerEvent<engine::event::KeyReleased>(events);
    registerEvent<engine::event::MouseMoved>(events);
    registerEvent<engine::event::MouseButtonPressed>(events);
    registerEvent<engine::event::MouseButtonReleased>(events);
    registerEvent<engine::event::MouseScrolled>(events);
    registerEvent<engine::event::GamepadConnected>(events);
    registerEvent<engine::event::GamepadDisconnected>(events);
    registerEvent<engine::event::GamepadButtonPressed>(events);
    registerEvent<engine::event::GamepadButtonReleased>(events);
    registerEvent<engine::event::GamepadAxisMoved>(events);

    ecs::Resources& resources = app.getWorld().getResources();
    resources.insertNonSend(BackendSetup{.registry = std::move(_registry), .settings = std::move(_settings)});
    resources.insertNonSend(engine::backend::Backend{});
    if (!resources.has<ClearColor>()) {  // a colour the game set before keeps
        resources.insert(ClearColor{});
    }

    app.addSystem(ecs::Phase::kStartup, std::string{kCreateSystem}, &createBackend).setSide(ecs::Side::kClient);
    ecs::Schedule::SystemConfig forward =
        app.addSystem(ecs::Phase::kPreUpdate, std::string{kEventsSystem}, &forwardEvents).setSide(ecs::Side::kClient);
    // Input reads the events this system sends, in the same phase. A constraint naming a missing system is an error,
    // so it is set by whichever of the two plugins is added second.
    if (app.hasPlugin(input::InputPlugin::kName)) {
        forward.before(std::string{input::InputPlugin::kUpdateSystem});
    }
    app.addSystem(ecs::Phase::kRender, std::string{kBeginFrameSystem}, &beginFrame).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kEndFrameSystem}, &endFrame)
        .after(std::string{kBeginFrameSystem})
        .setSide(ecs::Side::kClient);
}
}  // namespace rtype::plugins::backend
