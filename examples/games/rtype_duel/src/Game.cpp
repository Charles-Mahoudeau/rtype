/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Game
*/

#include "Game.hpp"

#include <cstdint>
#include <string>

#include "Components.hpp"
#include "Constants.hpp"
#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/replication/input/InputKind.hpp"
#include "rtype/replication/input/InputLayout.hpp"

namespace rtype_duel {

bool isOver(rtype::ecs::Query<const Winner>& winners) {
    return winners.begin() != rtype::ecs::Query<const Winner>::end();
}

float directionOf(std::uint32_t player) { return player == 0 ? 1.0F : -1.0F; }

std::uint32_t shipLayerOf(std::uint32_t player) { return player == 0 ? kLeftShipLayer : kRightShipLayer; }

rtype::plugins::physics::Collider shipCollider(std::uint32_t player) {
    return rtype::plugins::physics::Collider{.shape = rtype::plugins::physics::ShapeType::kBox,
                                             .size = kShipHalfSize * 2.0F,
                                             .layer = shipLayerOf(player),
                                             .mask = kObstacleLayer | kLaserLayer};
}

rtype::plugins::physics::Collider laserCollider(std::uint32_t shooter) {
    return rtype::plugins::physics::Collider{.shape = rtype::plugins::physics::ShapeType::kBox,
                                             .size = kLaserHalfSize * 2.0F,
                                             .layer = kLaserLayer,
                                             .mask = kObstacleLayer | shipLayerOf(shooter == 0 ? 1 : 0)};
}

void registerGame(rtype::ecs::App& app) {
    // A top-down arena: no gravity. The server simulates; a client's bodies only follow what it receives.
    app.addPlugin(
        rtype::plugins::physics::PhysicsPlugin{rtype::plugins::physics::PhysicsSettings{.gravity = {0.0F, 0.0F}}});
    rtype::ecs::Components& components = app.getWorld().getComponents();
    static_cast<void>(components.registerType<Ship>({.replication = rtype::ecs::ReplicationMode::kEveryChange}));
    static_cast<void>(components.registerType<Laser>({.replication = rtype::ecs::ReplicationMode::kEveryChange}));
    static_cast<void>(components.registerType<Obstacle>({.replication = rtype::ecs::ReplicationMode::kEveryChange}));
    static_cast<void>(components.registerType<Winner>({.replication = rtype::ecs::ReplicationMode::kEveryChange}));
    static_cast<void>(components.registerType<Misfire>({.replication = rtype::ecs::ReplicationMode::kEveryChange}));
}

void declareInputs(rtype::ecs::App& app) {
    rtype::replication::input::InputLayout& layout =
        *app.getWorld().getResources().getMut<rtype::replication::input::InputLayout>();
    layout.add(std::string{kMoveAction}, rtype::replication::input::InputKind::kVector2);
    layout.add(std::string{kShootAction}, rtype::replication::input::InputKind::kButton);
}

}  // namespace rtype_duel
