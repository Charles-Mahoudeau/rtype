/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsPlugin
*/

#include "plugins/physics/PhysicsPlugin.hpp"

#include <string>
#include <string_view>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/time/Time.hpp"

namespace rtype::plugins::physics {
namespace {
/// @brief FixedUpdate: steps the world by one fixed tick.
void step(ecs::ResMut<PhysicsWorld> world, ecs::Res<ecs::Time> time) { world->step(time->fixedDelta); }
}  // namespace

PhysicsPlugin::PhysicsPlugin(PhysicsSettings settings) : _settings{settings} {}

PhysicsPlugin::PhysicsPlugin() : PhysicsPlugin{PhysicsSettings{}} {}

std::string_view PhysicsPlugin::getName() const noexcept { return kName; }

void PhysicsPlugin::build(ecs::App& app) {
    static_cast<void>(app.getWorld().getComponents().registerType<RigidBody>());
    static_cast<void>(app.getWorld().getComponents().registerType<Collider>());
    app.insertResource(PhysicsWorld{_settings});
    app.addSystem(ecs::Phase::kFixedUpdate, std::string{kStepSystem}, &step);
}

}  // namespace rtype::plugins::physics
