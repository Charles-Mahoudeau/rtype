/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Server
*/

#include "Server.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "Components.hpp"
#include "Constants.hpp"
#include "Game.hpp"
#include "Resources.hpp"
#include "plugins/physics/Collider.hpp"
#include "plugins/physics/CollisionEvents.hpp"
#include "plugins/physics/MoveAndSlide.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/Velocity.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/time/Time.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/replication/ReplicationSettings.hpp"
#include "rtype/replication/component/Owner.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/event/ClientConnected.hpp"
#include "rtype/replication/event/ClientDisconnected.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/server/ServerPlugin.hpp"

using namespace rtype;

namespace rtype_duel {
namespace {

/// @brief Startup: the obstacles in the middle, static colliders: two walls and a rock between them.
void spawnObstacles(ecs::Commands& commands) {
    const std::array obstacles{
        std::pair{glm::vec2{kMiddleX, 140.0F},
                  plugins::physics::Collider{.shape = plugins::physics::ShapeType::kBox, .size = {40.0F, 140.0F}}},
        std::pair{glm::vec2{kMiddleX, kViewHeight / 2.0F},
                  plugins::physics::Collider{.shape = plugins::physics::ShapeType::kCircle, .radius = 36.0F}},
        std::pair{glm::vec2{kMiddleX, kViewHeight - 140.0F},
                  plugins::physics::Collider{.shape = plugins::physics::ShapeType::kBox, .size = {40.0F, 140.0F}}},
    };
    for (auto [position, collider] : obstacles) {
        collider.layer = kObstacleLayer;
        commands.spawn()
            .add(Obstacle{.position = position})
            .add(plugins::transform::Transform{.position = position})
            .add(collider);
    }
}

/// @brief Update: gives each of the first two clients a ship.
void spawnPlayers(ecs::Commands& commands, ecs::EventReader<replication::event::ClientConnected> connected,
                  ecs::Query<const Ship> ships) {
    std::uint32_t taken = 0;
    bool leftTaken = false;
    for (auto [entity, ship] : ships) {
        ++taken;
        leftTaken = leftTaken || ship.player == 0;
    }
    for (const replication::event::ClientConnected& client : connected) {
        if (taken >= kPlayerCount) {
            std::cout << "[server] client " << client.client << " joined, but the game is full\n";
            continue;
        }
        const std::uint32_t player = leftTaken ? 1 : 0;
        leftTaken = true;
        ++taken;
        const float x = player == 0 ? kShipX : kViewWidth - kShipX;
        const glm::vec2 position{x, kViewHeight / 2.0F};
        // Kinematic: moveShips moves it, the lasers hit it, nothing pushes it.
        commands.spawn()
            .add(Ship{.position = position, .player = player})
            .add(replication::component::Owner{.client = client.client})
            .add(replication::component::PlayerInput{})
            .add(plugins::transform::Transform{.position = position})
            .add(shipCollider(player))
            .add(plugins::physics::RigidBody{.type = plugins::physics::BodyType::kKinematic});
        std::cout << "[server] client " << client.client << " is player " << player + 1 << '\n';
    }
}

/// @brief Update: removes the ship of a client who left.
void removePlayers(ecs::Commands& commands, ecs::EventReader<replication::event::ClientDisconnected> disconnected,
                   ecs::Query<const replication::component::Owner, ecs::With<Ship>> owners) {
    for (const replication::event::ClientDisconnected& client : disconnected) {
        for (auto [entity, owner] : owners) {
            if (owner.client == client.client) {
                commands.despawn(entity);
                std::cout << "[server] client " << client.client << " left\n";
            }
        }
    }
}

/// @brief FixedUpdate, before the Transforms drive the bodies: moves the ships in their half of the arena, sliding
/// along the obstacles, and fires on a click (one shot in kMisfireOdds misfires), until someone has won.
void moveShips(ecs::Commands& commands, ecs::ResMut<Randomness> randomness, ecs::Res<ecs::Time> time,
               ecs::Res<replication::input::InputLayout> layout, ecs::Res<plugins::physics::PhysicsWorld> world,
               ecs::Query<Ship, plugins::transform::Transform, const plugins::physics::Collider,
                          const replication::component::PlayerInput>
                   ships,
               ecs::Query<const Winner> winners) {
    if (isOver(winners)) {
        return;
    }
    for (auto [entity, ship, transform, collider, input] : ships) {
        const std::array<float, 2> move = layout->readVector(input, kMoveAction);
        // Up is +1 in the input, and Y goes down on screen.
        const glm::vec2 delta = glm::vec2{move.at(0), -move.at(1)} * kShipSpeed * time->fixedDelta;
        const glm::vec2 slid = plugins::physics::moveAndSlide(*world, entity, collider, transform->position, delta);
        const bool left = ship->player == 0;
        const float minX = left ? kShipHalfSize.x : kMiddleX + kShipHalfSize.x;
        const float maxX = left ? kMiddleX - kShipHalfSize.x : kViewWidth - kShipHalfSize.x;
        transform->position = {std::clamp(slid.x, minX, maxX),
                               std::clamp(slid.y, kShipHalfSize.y, kViewHeight - kShipHalfSize.y)};
        ship->position = transform->position;
        if (layout->isPressed(input, kShootAction)) {
            const float direction = directionOf(ship->player);
            const glm::vec2 muzzle{ship->position.x + (direction * (kShipHalfSize.x + kLaserHalfSize.x + 1.0F)),
                                   ship->position.y};
            std::uniform_int_distribution<std::uint32_t> roll{1, kMisfireOdds};
            if (roll(randomness->engine) == 1) {
                commands.spawn()
                    .add(Misfire{.position = muzzle, .shooter = ship->player})
                    .add(MisfireTimer{.remaining = kMisfireLifetime});
                continue;
            }
            // A dynamic body flying at its Velocity, without gravity, never tunnelling through a thin wall.
            commands.spawn()
                .add(Laser{.position = muzzle, .shooter = ship->player})
                .add(plugins::transform::Transform{.position = muzzle})
                .add(laserCollider(ship->player))
                .add(plugins::physics::RigidBody{
                    .gravityScale = 0.0F, .fixedRotation = true, .continuousCollision = true})
                .add(plugins::physics::Velocity{.linear = {direction * kLaserSpeed, 0.0F}});
        }
    }
}

/// @brief FixedUpdate, after the physics moved the lasers: replicates where they are and removes those that left the
/// view.
void followLaserBodies(ecs::Commands& commands, ecs::Query<Laser, const plugins::transform::Transform> lasers) {
    for (auto [entity, laser, transform] : lasers) {
        laser->position = transform.position;
        if (laser->position.x < -kLaserHalfSize.x || laser->position.x > kViewWidth + kLaserHalfSize.x) {
            commands.despawn(entity);
        }
    }
}

/// @brief FixedUpdate: removes the Misfires once a snapshot had time to carry them to the clients.
void expireMisfires(ecs::Commands& commands, ecs::Res<ecs::Time> time, ecs::Query<MisfireTimer> timers) {
    for (auto [entity, timer] : timers) {
        timer->remaining -= time->fixedDelta;
        if (timer->remaining <= 0.0F) {
            commands.despawn(entity);
        }
    }
}

/// @return The shooter of the laser, or nothing if the entity is not a laser.
std::optional<std::uint32_t> shooterOf(ecs::Query<const Laser>& lasers, ecs::Entity entity) {
    for (auto [laserEntity, laser] : lasers) {
        if (laserEntity == entity) {
            return laser.shooter;
        }
    }
    return std::nullopt;
}

/// @return Whether the entity is a ship.
bool isShip(ecs::Query<const Ship>& ships, ecs::Entity entity) {
    return std::ranges::any_of(ships, [entity](const auto& row) { return std::get<0>(row) == entity; });
}

/// @brief FixedUpdate, after the physics sent its events: a laser that touches something is spent; one that touches a
/// ship (always the other player's, by the collision layers) wins the game.
void detectHits(ecs::Commands& commands, ecs::EventReader<plugins::physics::CollisionStarted> collisions,
                ecs::Query<const Laser> lasers, ecs::Query<const Ship> ships, ecs::Query<const Winner> winners) {
    bool over = isOver(winners);
    std::vector<ecs::Entity> spent;
    for (const plugins::physics::CollisionStarted& collision : collisions) {
        for (const auto& [laser, other] :
             {std::pair{collision.first, collision.second}, std::pair{collision.second, collision.first}}) {
            const std::optional<std::uint32_t> shooter = shooterOf(lasers, laser);
            if (!shooter || std::ranges::find(spent, laser) != spent.end()) {
                continue;
            }
            spent.push_back(laser);
            commands.despawn(laser);
            if (!over && isShip(ships, other)) {
                over = true;
                commands.spawn().add(Winner{.player = *shooter});
                std::cout << "[server] player " << *shooter + 1 << " wins\n";
            }
        }
    }
}

}  // namespace

int runServer(std::uint16_t port) {
    ecs::App app{ecs::RunMode::kServer};
    app.addPlugins(ecs::MinimalPlugins{});
    registerGame(app);
    network::NetworkSettings network;
    network.port = port;
    app.addPlugin(replication::server::ServerPlugin{network, replication::ReplicationSettings{}});
    declareInputs(app);
    app.insertResource(Randomness{});
    static_cast<void>(app.getWorld().getComponents().registerType<MisfireTimer>());

    app.addSystem(ecs::Phase::kStartup, "game.spawnObstacles", &spawnObstacles);
    app.addSystem(ecs::Phase::kUpdate, "game.spawnPlayers", &spawnPlayers);
    app.addSystem(ecs::Phase::kUpdate, "game.removePlayers", &removePlayers);
    // Around the physics systems: the ships move before the Transforms drive the bodies, the hits are read after the
    // step sent its events.
    app.addSystem(ecs::Phase::kFixedUpdate, "game.moveShips", &moveShips)
        .after(std::string{replication::server::ServerPlugin::kApplyInputsSystem})
        .after(std::string{plugins::physics::PhysicsPlugin::kSyncBodiesSystem})
        .before(std::string{plugins::physics::PhysicsPlugin::kSyncToPhysicsSystem});
    app.addSystem(ecs::Phase::kFixedUpdate, "game.followLaserBodies", &followLaserBodies)
        .after(std::string{plugins::physics::PhysicsPlugin::kSyncFromPhysicsSystem});
    app.addSystem(ecs::Phase::kFixedUpdate, "game.expireMisfires", &expireMisfires);
    app.addSystem(ecs::Phase::kFixedUpdate, "game.detectHits", &detectHits)
        .after(std::string{plugins::physics::PhysicsPlugin::kEventsSystem})
        .after("game.followLaserBodies");
    std::cout << "[server] listening on port " << port << ", waiting for " << kPlayerCount << " players\n";
    return app.run();
}

}  // namespace rtype_duel
