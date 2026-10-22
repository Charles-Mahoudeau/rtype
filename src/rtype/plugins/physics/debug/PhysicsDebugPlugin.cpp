/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsDebugPlugin
*/

#include "plugins/physics/debug/PhysicsDebugPlugin.hpp"

#include <string>
#include <string_view>

#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/BackendResources.hpp"  // IWYU pragma: keep
#include "plugins/backend/event/Event.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/IRenderer.hpp"
#include "plugins/backend/graphics/Shape.hpp"
#include "plugins/backend/input/Key.hpp"
#include "plugins/input/InputEvents.hpp"  // IWYU pragma: keep
#include "plugins/physics/Collider.hpp"
#include "plugins/physics/ComponentOptions.hpp"
#include "plugins/physics/Velocity.hpp"
#include "plugins/physics/debug/PhysicsDebug.hpp"
#include "plugins/physics/debug/PhysicsDebugSettings.hpp"
#include "plugins/physics/debug/PhysicsDebugShapes.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"

namespace rtype::plugins::physics::debug {
namespace {
/// @brief Update: flips PhysicsDebugSettings::enabled when the toggle key is pressed (not on auto-repeat).
void toggleDebug(ecs::ResMut<PhysicsDebugSettings> settings, ecs::EventReader<backend::event::KeyPressed> pressed) {
    const backend::input::Key toggleKey = settings.get().toggleKey;
    for (const backend::event::KeyPressed& event : pressed) {
        if (toggleKey != backend::input::Key::kUnknown && event.key == toggleKey && !event.repeat) {
            settings->enabled = !settings.get().enabled;
        }
    }
}

/// @return The colour an entity is drawn in, or nothing when it is not drawn: PhysicsDebug chooses it, drawAll draws
/// the others in the settings' colour.
const backend::graphics::Color* colorOf(const PhysicsDebug* debug, bool wanted, const PhysicsDebugSettings& settings) {
    if (debug != nullptr) {
        return wanted || settings.drawAll ? &debug->color : nullptr;
    }
    return settings.drawAll ? &settings.color : nullptr;
}

/// @brief Render, after the sprites: draws the chosen entities' collider outlines and velocity arrows.
void drawDebug(
    ecs::NonSendMut<backend::Backend> backend, ecs::Res<PhysicsDebugSettings> settings,
    ecs::Query<const Collider, const transform::GlobalTransform, ecs::Optional<const PhysicsDebug>> colliders,
    ecs::Query<const Velocity, const transform::GlobalTransform, ecs::Optional<const PhysicsDebug>> moving) {
    if (!settings->enabled || backend->renderer == nullptr) {
        return;
    }
    backend::graphics::IRenderer& renderer = *backend->renderer;
    for (auto [entity, collider, transform, debug] : colliders) {
        if (const backend::graphics::Color* color = colorOf(debug, debug != nullptr && debug->shape, *settings)) {
            renderer.draw(debugColliderShape(collider, transform, *color, settings->outlineThickness));
        }
    }
    for (auto [entity, velocity, transform, debug] : moving) {
        if (const backend::graphics::Color* color = colorOf(debug, debug != nullptr && debug->velocity, *settings)) {
            for (const backend::graphics::Shape& stroke :
                 debugVelocityArrow(transform.position, velocity.linear, settings->velocityScale, *color,
                                    settings->outlineThickness)) {
                renderer.draw(stroke);
            }
        }
    }
}
}  // namespace

std::string_view PhysicsDebugPlugin::getName() const noexcept { return kName; }

void PhysicsDebugPlugin::build(ecs::App& app) {
    app.addPlugin(transform::TransformPlugin{});
    static_cast<void>(app.getWorld().getComponents().registerType<Collider>(kColliderOptions));
    static_cast<void>(app.getWorld().getComponents().registerType<Velocity>());
    static_cast<void>(app.getWorld().getComponents().registerType<PhysicsDebug>());
    static_cast<void>(app.getWorld().getEvents().registerType<backend::event::KeyPressed>());
    ecs::Resources& resources = app.getWorld().getResources();
    if (!resources.has<PhysicsDebugSettings>()) {
        resources.insert(PhysicsDebugSettings{});
    }
    app.addSystem(ecs::Phase::kUpdate, std::string{kToggleSystem}, &toggleDebug).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kDrawSystem}, &drawDebug)
        .after(std::string{backend::BackendPlugin::kSpritesSystem})
        .before(std::string{backend::BackendPlugin::kEndFrameSystem})
        .setSide(ecs::Side::kClient);
}

}  // namespace rtype::plugins::physics::debug
