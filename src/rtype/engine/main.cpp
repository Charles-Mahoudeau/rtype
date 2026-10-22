/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <glm/ext/vector_float2.hpp>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "platform/glfw/Registration.hpp"
#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/BackendResources.hpp"  // IWYU pragma: keep
#include "plugins/backend/config/Settings.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/IRenderer.hpp"
#include "plugins/backend/graphics/Shape.hpp"
#include "plugins/backend/graphics/SpriteRenderer.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/backend/input/Key.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "plugins/input/Control.hpp"
#include "plugins/input/Input.hpp"
#include "plugins/input/InputAction.hpp"
#include "plugins/physics/Collider.hpp"
#include "plugins/physics/CollisionEvents.hpp"
#include "plugins/physics/MoveAndSlide.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/Velocity.hpp"
#include "plugins/physics/debug/PhysicsDebugPlugin.hpp"
#include "plugins/physics/debug/PhysicsDebugSettings.hpp"
#include "plugins/transform/Transform.hpp"
#include "render/vulkan/Registration.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/AppExit.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/time/Time.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/replication/ReplicationSettings.hpp"
#include "rtype/replication/client/ClientPlugin.hpp"
#include "rtype/replication/client/ClientState.hpp"
#include "rtype/replication/component/Owner.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/event/ClientConnected.hpp"
#include "rtype/replication/event/ClientDisconnected.hpp"
#include "rtype/replication/input/InputKind.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/server/ServerPlugin.hpp"

// A two-player duel, the most basic game the engine can run: each player moves a ship in their half of the arena and
// fires lasers; the first laser that touches the other ship wins. Obstacles in the middle stop the lasers, and the
// ships slide along them. The server simulates everything with the physics plugin and replicates Ship, Laser,
// Obstacle and Winner (the physics replicates Collider and RigidBody); the clients send their inputs and draw what
// they receive. F3 shows the colliders.
//
//   engine server [port]           headless server, waits for two clients
//   engine client [host] [port]    a player (default 127.0.0.1:4242)

namespace rtype::engine {
namespace {

/// @brief A player's ship: replicated on every change.
struct Ship {
    glm::vec2 position{0.0F};  ///< In world units (the 800x600 view).
    std::uint32_t player{0};   ///< 0 on the left, firing right; 1 on the right, firing left.
};
RTYPE_ECS_COMPONENT(Ship, "game.Ship", position, player);

/// @brief A laser in flight: replicated on every change.
struct Laser {
    glm::vec2 position{0.0F};  ///< In world units.
    std::uint32_t shooter{0};  ///< The player who fired it: it flies away from them and cannot hit them.
};
RTYPE_ECS_COMPONENT(Laser, "game.Laser", position, shooter);

/// @brief A wall in the middle of the arena, spawned by the server: replicated on every change.
struct Obstacle {
    glm::vec2 position{0.0F};  ///< In world units: the clients place it with a Transform.
};
RTYPE_ECS_COMPONENT(Obstacle, "game.Obstacle", position);

/// @brief Spawned by the server when a laser hits: the game is over.
struct Winner {
    std::uint32_t player{0};  ///< The player whose laser hit.
};
RTYPE_ECS_COMPONENT(Winner, "game.Winner", player);

/// @brief Client only: plays the frames of a sprite sheet laid out in a row, by changing the SpriteRenderer's source.
struct SpriteAnimation {
    glm::vec2 firstFrame{0.0F};  ///< Top-left corner of the first frame, in pixels.
    glm::vec2 frameSize{0.0F};   ///< Size of a frame, in pixels.
    float stride = 0.0F;         ///< Pixels from one frame's left edge to the next.
    std::uint32_t frameCount = 1;
    float frameDuration = 0.1F;  ///< Seconds per frame.
    float elapsed = 0.0F;        ///< Seconds since the animation started.
};
RTYPE_ECS_COMPONENT(SpriteAnimation, "game.SpriteAnimation");

constexpr std::uint16_t kDefaultPort = 4242;
constexpr float kViewWidth = 800.0F;
constexpr float kViewHeight = 600.0F;
constexpr float kShipX = 80.0F;        ///< Distance of each ship from its edge.
constexpr float kShipSpeed = 300.0F;   ///< World units per second.
constexpr float kLaserSpeed = 600.0F;  ///< World units per second.
constexpr glm::vec2 kShipHalfSize{24.0F, 24.0F};
constexpr glm::vec2 kLaserHalfSize{30.0F, 6.0F};
constexpr std::uint32_t kPlayerCount = 2;
constexpr float kMiddleX = kViewWidth / 2.0F;  ///< Each ship stays on its side of it.

// The collision categories: each Collider belongs to some (layer) and collides with some (mask).
constexpr std::uint32_t kObstacleLayer = 1U << 0;
constexpr std::uint32_t kLaserLayer = 1U << 1;
constexpr std::uint32_t kLeftShipLayer = 1U << 2;
constexpr std::uint32_t kRightShipLayer = 1U << 3;

constexpr plugins::backend::graphics::Color kObstacleFill{0.25F, 0.27F, 0.32F, 1.0F};
constexpr plugins::backend::graphics::Color kObstacleOutline{0.55F, 0.6F, 0.7F, 1.0F};

constexpr std::string_view kMoveAction = "move";    ///< Up and down, a vector (only Y is used).
constexpr std::string_view kShootAction = "shoot";  ///< The left mouse button.

constexpr const char* kLeftShipSheet = "assets/r-typesheet24.gif";
constexpr const char* kRightShipSheet = "assets/r-typesheet26.gif";
constexpr const char* kLaserSheet = "assets/bullet.gif";

/// @return Whether a Winner exists: the game is over.
bool isOver(ecs::Query<const Winner>& winners) { return winners.begin() != ecs::Query<const Winner>::end(); }

/// @return +1 for the left player (fires right), -1 for the right one.
float directionOf(std::uint32_t player) { return player == 0 ? 1.0F : -1.0F; }

/// @return The collision layer of a player's ship.
std::uint32_t shipLayerOf(std::uint32_t player) { return player == 0 ? kLeftShipLayer : kRightShipLayer; }

/// @return A ship's collider: blocked by the obstacles, hit by the lasers.
plugins::physics::Collider shipCollider(std::uint32_t player) {
    return plugins::physics::Collider{.shape = plugins::physics::ShapeType::kBox,
                                      .size = kShipHalfSize * 2.0F,
                                      .layer = shipLayerOf(player),
                                      .mask = kObstacleLayer | kLaserLayer};
}

/// @return A laser's collider: it touches the obstacles and the other player's ship, never its shooter.
plugins::physics::Collider laserCollider(std::uint32_t shooter) {
    return plugins::physics::Collider{.shape = plugins::physics::ShapeType::kBox,
                                      .size = kLaserHalfSize * 2.0F,
                                      .layer = kLaserLayer,
                                      .mask = kObstacleLayer | shipLayerOf(shooter == 0 ? 1 : 0)};
}

// ---------------------------------------------------------------------------------------------------------------
// Shared by the server and the clients: the manifest checks they register the same things.

/// @brief Adds the physics (which replicates Collider and RigidBody) and registers the game's replicated components.
void registerGame(ecs::App& app) {
    // A top-down arena: no gravity. The server simulates; a client's bodies only follow what it receives.
    app.addPlugin(plugins::physics::PhysicsPlugin{plugins::physics::PhysicsSettings{.gravity = {0.0F, 0.0F}}});
    ecs::Components& components = app.getWorld().getComponents();
    static_cast<void>(components.registerType<Ship>({.replication = ecs::ReplicationMode::kEveryChange}));
    static_cast<void>(components.registerType<Laser>({.replication = ecs::ReplicationMode::kEveryChange}));
    static_cast<void>(components.registerType<Obstacle>({.replication = ecs::ReplicationMode::kEveryChange}));
    static_cast<void>(components.registerType<Winner>({.replication = ecs::ReplicationMode::kEveryChange}));
}

/// @brief Declares the input actions both sides agree on; after the server or client plugin inserted the layout.
void declareInputs(ecs::App& app) {
    replication::input::InputLayout& layout = *app.getWorld().getResources().getMut<replication::input::InputLayout>();
    layout.add(std::string{kMoveAction}, replication::input::InputKind::kVector2);
    layout.add(std::string{kShootAction}, replication::input::InputKind::kButton);
}

// ---------------------------------------------------------------------------------------------------------------
// Server.

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
/// along the obstacles, and fires on a click, until someone has won.
void moveShips(ecs::Commands& commands, ecs::Res<ecs::Time> time, ecs::Res<replication::input::InputLayout> layout,
               ecs::Res<plugins::physics::PhysicsWorld> world,
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
    for (auto [shipEntity, ship] : ships) {
        if (shipEntity == entity) {
            return true;
        }
    }
    return false;
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

/// @brief Runs the headless server until it is stopped.
int runServer(std::uint16_t port) {
    ecs::App app{ecs::RunMode::kServer};
    app.addPlugins(ecs::MinimalPlugins{});
    registerGame(app);
    network::NetworkSettings network;
    network.port = port;
    app.addPlugin(replication::server::ServerPlugin{network, replication::ReplicationSettings{}});
    declareInputs(app);

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
    app.addSystem(ecs::Phase::kFixedUpdate, "game.detectHits", &detectHits)
        .after(std::string{plugins::physics::PhysicsPlugin::kEventsSystem})
        .after("game.followLaserBodies");
    std::cout << "[server] listening on port " << port << ", waiting for " << kPlayerCount << " players\n";
    return app.run();
}

// ---------------------------------------------------------------------------------------------------------------
// Client.

/// @brief Startup: binds the local controls: W / A / S / D or the arrows to move, the left mouse button to fire.
void bindControls(ecs::ResMut<plugins::input::Input> input) {
    using plugins::backend::input::Key;
    using plugins::input::Control;
    input->addAction(std::string{kMoveAction}, plugins::input::ActionType::kVector2)
        .bindVector(Control::key(Key::kW), Control::key(Key::kS), Control::key(Key::kA), Control::key(Key::kD))
        .bindVector(Control::key(Key::kUp), Control::key(Key::kDown), Control::key(Key::kLeft),
                    Control::key(Key::kRight));
    input->addAction(std::string{kShootAction}, plugins::input::ActionType::kButton)
        .bind(Control::mouseButton(plugins::backend::input::MouseButton::kLeft));
}

/// @brief PostUpdate, before the input is sent: copies the local controls into the network input.
void sendControls(ecs::Res<plugins::input::Input> input, ecs::Res<replication::input::InputLayout> layout,
                  ecs::ResMut<replication::client::ClientState> client) {
    replication::component::PlayerInput sent;
    const glm::vec2 move = input->getAction(std::string{kMoveAction}).readVector();
    layout->setVector(sent, kMoveAction, {move.x, move.y});
    layout->setButton(sent, kShootAction, input->getAction(std::string{kShootAction}).isHeld());
    client->setInput(sent);
}

/// @brief Update: gives the ships and lasers the server created a Transform, a sprite and an animation.
void dressEntities(ecs::Commands& commands, ecs::ResMut<plugins::backend::TextureAssets> textures,
                   ecs::Query<const Ship, ecs::Without<plugins::backend::SpriteRenderer>> ships,
                   ecs::Query<const Laser, ecs::Without<plugins::backend::SpriteRenderer>> lasers) {
    for (auto [entity, ship] : ships) {
        const bool left = ship.player == 0;
        // The left sheet has 5 frames of 48x48 every 65 pixels; the right one 6 frames of 63x48, about every 65.4.
        const SpriteAnimation animation =
            left ? SpriteAnimation{.firstFrame = {9.0F, 9.0F},
                                   .frameSize = {48.0F, 48.0F},
                                   .stride = 65.0F,
                                   .frameCount = 5}
                 : SpriteAnimation{
                       .firstFrame = {1.0F, 1.0F}, .frameSize = {63.0F, 48.0F}, .stride = 65.4F, .frameCount = 6};
        commands.entity(entity)
            .add(plugins::transform::Transform{.position = ship.position})
            .add(plugins::backend::SpriteRenderer{
                .texture = textures->load(left ? kLeftShipSheet : kRightShipSheet),
                .source = plugins::backend::math::Rect{animation.firstFrame, animation.frameSize},
                .origin = animation.frameSize / 2.0F,
                .layer = 1})
            .add(animation);
    }
    for (auto [entity, laser] : lasers) {
        // bullet.gif: two frames flying right from x 108, two flying left from x 271, 80x16 every 81 pixels.
        const SpriteAnimation animation{.firstFrame = {laser.shooter == 0 ? 108.0F : 271.0F, 151.0F},
                                        .frameSize = {80.0F, 16.0F},
                                        .stride = 81.0F,
                                        .frameCount = 2,
                                        .frameDuration = 0.08F};
        commands.entity(entity)
            .add(plugins::transform::Transform{.position = laser.position})
            .add(plugins::backend::SpriteRenderer{
                .texture = textures->load(kLaserSheet),
                .source = plugins::backend::math::Rect{animation.firstFrame, animation.frameSize},
                .origin = animation.frameSize / 2.0F,
                .layer = 2})
            .add(animation);
    }
}

/// @brief Update: places the obstacles the server created, which the physics debug view and drawObstacles() read.
void placeObstacles(ecs::Commands& commands,
                    ecs::Query<const Obstacle, ecs::Without<plugins::transform::Transform>> obstacles) {
    for (auto [entity, obstacle] : obstacles) {
        commands.entity(entity).add(plugins::transform::Transform{.position = obstacle.position});
    }
}

/// @brief Render, after the sprites: draws each obstacle as its collider's shape, filled.
void drawObstacles(
    ecs::NonSendMut<plugins::backend::Backend> backend,
    ecs::Query<const plugins::physics::Collider, const plugins::transform::GlobalTransform, ecs::With<Obstacle>>
        obstacles) {
    if (backend->renderer == nullptr) {
        return;
    }
    for (auto [entity, collider, transform] : obstacles) {
        const bool round = collider.shape == plugins::physics::ShapeType::kCircle;
        backend->renderer->draw(
            plugins::backend::graphics::Shape{.kind = round ? plugins::backend::graphics::ShapeKind::kCircle
                                                            : plugins::backend::graphics::ShapeKind::kRect,
                                              .position = transform.position + collider.offset,
                                              .size = collider.size,
                                              .radius = collider.radius,
                                              .cornerRadius = 6.0F,
                                              .fillColor = kObstacleFill,
                                              .outlineColor = kObstacleOutline,
                                              .outlineThickness = 2.0F});
    }
}

/// @brief Update: places each ship's sprite where the server says the ship is.
void followShips(ecs::Query<plugins::transform::Transform, const Ship> ships) {
    for (auto [entity, transform, ship] : ships) {
        transform->position = ship.position;
    }
}

/// @brief Update: places each laser's sprite where the server says the laser is.
void followLasers(ecs::Query<plugins::transform::Transform, const Laser> lasers) {
    for (auto [entity, transform, laser] : lasers) {
        transform->position = laser.position;
    }
}

/// @brief Update: shows each animation's current frame.
void animateSprites(ecs::Res<ecs::Time> time, ecs::Query<plugins::backend::SpriteRenderer, SpriteAnimation> sprites) {
    for (auto [entity, sprite, animation] : sprites) {
        animation->elapsed += time->delta;
        const auto frame =
            static_cast<std::uint32_t>(animation->elapsed / animation->frameDuration) % animation->frameCount;
        const glm::vec2 corner{animation->firstFrame.x + (static_cast<float>(frame) * animation->stride),
                               animation->firstFrame.y};
        sprite->source = plugins::backend::math::Rect{corner, animation->frameSize};
    }
}

/// @brief Update: tells the player who won, once.
void announceWinner(ecs::Res<replication::client::ClientState> client,
                    ecs::Query<const Winner, ecs::Added<Winner>> winners,
                    ecs::Query<const Ship, const replication::component::Owner> ships) {
    const std::optional<network::ClientId> me = client->clientId();
    for (auto [entity, winner] : winners) {
        bool mine = false;
        for (auto [shipEntity, ship, owner] : ships) {
            mine = mine || (me && ship.player == winner.player && owner.client == *me);
        }
        std::cout << "Player " << winner.player + 1 << " wins! " << (mine ? "You win :)" : "You lose :(") << '\n';
    }
}

/// @brief Update: stops the App when Escape is pressed.
void quitOnEscape(ecs::Res<plugins::input::Input> input, ecs::EventWriter<ecs::AppExit> exit) {
    if (input->read(plugins::input::Control::key(plugins::backend::input::Key::kEscape)) > 0.0F) {
        exit.send(ecs::AppExit{.code = 0});
    }
}

/// @return The configuration of the window and the renderer.
plugins::backend::config::Settings makeSettings() {
    plugins::backend::config::Settings settings;
    settings.set("platform", "glfw");
    settings.set("renderer", "vulkan");
    settings.set("window.title", "R-Type duel");
    settings.set("window.size", std::vector<double>{kViewWidth, kViewHeight});
    // The world the game is designed for, in world units: the same on a HiDPI screen and in a resized window.
    settings.set("vulkan.virtualResolution", std::vector<double>{kViewWidth, kViewHeight});
#ifndef NDEBUG
    // Debug builds only: the validation layer (with synchronization validation) is slow.
    settings.set("vulkan.layers", std::vector<std::string>{"VK_LAYER_KHRONOS_validation"});
    settings.set("vulkan.debugging", true);
#endif
    return settings;
}

/// @brief Runs a player's window until it is closed or Escape is pressed.
int runClient(const std::string& host, std::uint16_t port) {
    const std::optional<network::io::Address> server = network::io::Address::resolve(host, port);
    if (!server) {
        std::cerr << "Error: cannot resolve " << host << '\n';
        return 1;
    }
    // The only place that knows GLFW and Vulkan: the plugins pick them by name from the settings.
    auto registry = std::make_unique<plugins::backend::BackendRegistry>();
    platform::glfw::registerPlatform(*registry);
    render::vulkan::registerRenderer(*registry);

    ecs::App app{ecs::RunMode::kClient};
    app.addPlugins(ecs::MinimalPlugins{})
        .addPlugin(plugins::backend::BackendPlugin{std::move(registry), makeSettings()});
    registerGame(app);
    // F3 outlines every collider; hidden at first.
    app.insertResource(plugins::physics::debug::PhysicsDebugSettings{.enabled = false, .drawAll = true});
    app.addPlugin(plugins::physics::debug::PhysicsDebugPlugin{});
    static_cast<void>(app.getWorld().getComponents().registerType<SpriteAnimation>());
    app.addPlugin(replication::client::ClientPlugin{*server, network::NetworkSettings{}});
    declareInputs(app);

    app.addSystem(ecs::Phase::kStartup, "game.bindControls", &bindControls).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kPostUpdate, "game.sendControls", &sendControls)
        .before(std::string{replication::client::ClientPlugin::kSendInputSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kUpdate, "game.dressEntities", &dressEntities).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kUpdate, "game.placeObstacles", &placeObstacles).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, "game.drawObstacles", &drawObstacles)
        .after(std::string{plugins::backend::BackendPlugin::kSpritesSystem})
        .before(std::string{plugins::physics::debug::PhysicsDebugPlugin::kDrawSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kUpdate, "game.followShips", &followShips).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kUpdate, "game.followLasers", &followLasers)
        .after("game.followShips")
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kUpdate, "game.animateSprites", &animateSprites).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kUpdate, "game.announceWinner", &announceWinner).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kUpdate, "game.quitOnEscape", &quitOnEscape).setSide(ecs::Side::kClient);
    std::cout << "Connecting to " << host << ':' << port
              << "... WASD or the arrows to move, click to fire, F3 for the colliders.\n";
    return app.run();
}

/// @return @p text as a port, or the default one if it is empty or not a number in range.
std::uint16_t toPort(std::string_view text) {
    std::uint16_t port = 0;
    const std::string digits{text};
    const char* const last = std::to_address(digits.end());
    const auto [end, error] = std::from_chars(digits.c_str(), last, port);
    return error == std::errc{} && end == last && port != 0 ? port : kDefaultPort;
}

}  // namespace
}  // namespace rtype::engine

int main(int argc, char** argv) {
    try {
        const std::span<char*> args{argv, static_cast<std::size_t>(argc)};
        const auto arg = [&args](std::size_t index, std::string_view fallback) {
            return index < args.size() ? std::string_view{args.subspan(index).front()} : fallback;
        };
        const std::string_view mode = arg(1, "client");
        if (mode == "server") {
            return rtype::engine::runServer(rtype::engine::toPort(arg(2, "")));
        }
        if (mode == "client") {
            return rtype::engine::runClient(std::string{arg(2, "127.0.0.1")}, rtype::engine::toPort(arg(3, "")));
        }
        std::cerr << "Usage: engine server [port] | engine client [host] [port]\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
