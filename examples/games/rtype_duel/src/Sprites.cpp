/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Sprites
*/

#include "Sprites.hpp"

#include <format>
#include <glm/ext/vector_float2.hpp>
#include <utility>

#include "Constants.hpp"
#include "plugins/backend/graphics/BlendMode.hpp"
#include "plugins/backend/graphics/IRenderer.hpp"
#include "plugins/backend/graphics/Shape.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"

using namespace rtype;

namespace rtype_duel {

void loadAtlases(ecs::ResMut<plugins::sprite::AtlasAssets> atlases, ecs::ResMut<GameAtlases> game) {
    // The left sheet: 5 frames of 48x48, every 65 pixels.
    plugins::sprite::SpriteAtlas left =
        plugins::sprite::SpriteAtlas::grid(kLeftShipSheet, {48.0F, 48.0F}, 5, {17.0F, 0.0F}, 0, {9.0F, 9.0F});
    left.addAnimation("fly", {0, 1, 2, 3, 4}, 0.1F);
    // The right sheet: 3 frames of 63x48 facing left, every 65 pixels, then the same 3 mirrored (facing right).
    plugins::sprite::SpriteAtlas right =
        plugins::sprite::SpriteAtlas::grid(kRightShipSheet, {63.0F, 48.0F}, 3, {2.0F, 0.0F}, 0, {1.0F, 1.0F});
    right.addAnimation("fly", {0, 1, 2}, 0.1F, plugins::sprite::AnimationDirection::kPingPong);
    // Two frames of 80x16 flying right from x 108, then two flying left from x 271, every 81 pixels.
    plugins::sprite::SpriteAtlas laser{kLaserSheet};
    for (const float x : {108.0F, 189.0F, 271.0F, 352.0F}) {
        laser.addFrame(std::format("{}", laser.getFrames().size()),
                       plugins::backend::math::Rect{{x, 151.0F}, {80.0F, 16.0F}});
    }
    laser.addAnimation("right", {0, 1}, 0.08F);
    laser.addAnimation("left", {2, 3}, 0.08F);
    game->laser = atlases->add("game.laser", std::move(laser));
    game->leftShip = atlases->add("game.leftShip", std::move(left));
    game->rightShip = atlases->add("game.rightShip", std::move(right));
    // The big explosions, the bottom row: 10 frames of 64x64, every 65 pixels; the first six burst and fade.
    plugins::sprite::SpriteAtlas explosion =
        plugins::sprite::SpriteAtlas::grid(kExplosionSheet, {64.0F, 64.0F}, 10, {1.0F, 0.0F}, 0, {1.0F, 99.0F});
    explosion.addAnimation("explode", {0, 1, 2, 3, 4, 5}, 0.07F, plugins::sprite::AnimationDirection::kForward, 1);
    game->explosion = atlases->add("game.explosion", std::move(explosion));
}

void dressEntities(ecs::Commands& commands, ecs::Res<GameAtlases> game,
                   ecs::Query<const Ship, ecs::Without<plugins::backend::SpriteRenderer>> ships,
                   ecs::Query<const Laser, ecs::Without<plugins::backend::SpriteRenderer>> lasers) {
    for (auto [entity, ship] : ships) {
        commands.entity(entity)
            .add(plugins::transform::Transform{.position = ship.position})
            .add(plugins::backend::SpriteRenderer{.layer = 1})
            .add(plugins::sprite::SpriteAnimator{.atlas = ship.player == 0 ? game->leftShip : game->rightShip,
                                                 .animation = plugins::sprite::animationId("fly")});
    }
    for (auto [entity, laser] : lasers) {
        commands.entity(entity)
            .add(plugins::transform::Transform{.position = laser.position})
            .add(plugins::backend::SpriteRenderer{.layer = 2})
            .add(plugins::sprite::SpriteAnimator{
                .atlas = game->laser,
                .animation = plugins::sprite::animationId(laser.shooter == 0 ? "right" : "left")});
    }
}

void placeObstacles(ecs::Commands& commands,
                    ecs::Query<const Obstacle, ecs::Without<plugins::transform::Transform>> obstacles) {
    for (auto [entity, obstacle] : obstacles) {
        commands.entity(entity).add(plugins::transform::Transform{.position = obstacle.position});
    }
}

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

void followShips(ecs::Query<plugins::transform::Transform, const Ship> ships) {
    for (auto [entity, transform, ship] : ships) {
        transform->position = ship.position;
    }
}

void followLasers(ecs::Query<plugins::transform::Transform, const Laser> lasers) {
    for (auto [entity, transform, laser] : lasers) {
        transform->position = laser.position;
    }
}

void explodeLoser(ecs::Commands& commands, ecs::Res<GameAtlases> game,
                  ecs::Query<const Winner, ecs::Added<Winner>> winners,
                  ecs::Query<const Ship, plugins::backend::SpriteRenderer> ships) {
    for (auto [entity, winner] : winners) {
        for (auto [shipEntity, ship, sprite] : ships) {
            if (ship.player == winner.player) {
                continue;
            }
            sprite->color.w = 0.0F;  // Its alpha: the ship is gone.
            commands.spawn()
                .add(plugins::transform::Transform{.position = ship.position, .scale = glm::vec2{kExplosionScale}})
                .add(plugins::backend::SpriteRenderer{.layer = 3,
                                                      .blend = plugins::backend::graphics::BlendMode::kAdditive})
                .add(plugins::sprite::SpriteAnimator{.atlas = game->explosion,
                                                     .animation = plugins::sprite::animationId("explode")})
                .add(Explosion{});
        }
    }
}

void removeExplosions(ecs::Commands& commands, ecs::EventReader<plugins::sprite::AnimationFinished> finished,
                      ecs::Query<const plugins::sprite::SpriteAnimator, ecs::With<Explosion>> explosions) {
    for (const plugins::sprite::AnimationFinished& event : finished) {
        for (auto [entity, animator] : explosions) {
            if (entity == event.entity) {
                commands.despawn(entity);
            }
        }
    }
}

}  // namespace rtype_duel
