/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Background
*/

#include "Background.hpp"

#include <cmath>
#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <random>

#include "Constants.hpp"
#include "plugins/backend/graphics/SpriteRenderer.hpp"
#include "plugins/tilemap/SpawnTilemap.hpp"

using namespace rtype;

namespace rtype_duel {

void spawnBackground(ecs::ResMut<plugins::tilemap::TilemapAssets> maps, ecs::Commands& commands) {
    const float scale = kViewHeight / kBackgroundSize.y;
    // The map's parallax origin is its centre: put it where the camera looks, the centre of the view.
    const glm::vec2 view{kViewWidth, kViewHeight};
    static_cast<void>(plugins::tilemap::spawnTilemap(
        commands, *maps, maps->load(kBackgroundMap),
        plugins::tilemap::TilemapPlacement{.position = (view - (kBackgroundSize * scale)) / 2.0F,
                                           .scale = glm::vec2{scale},
                                           .firstDrawLayer = kBackgroundDrawLayer}));
}

void loadPlanets(ecs::ResMut<plugins::backend::TextureAssets> textures, ecs::ResMut<BackgroundPlanets> planets) {
    for (std::size_t index = 0; index < kPlanetImages.size(); ++index) {
        planets->images.at(index) = textures->load(kPlanetImages.at(index));
    }
    planets->countdown = kFirstPlanetDelay;
}

void spawnPlanets(ecs::Commands& commands, ecs::ResMut<Randomness> randomness, ecs::ResMut<BackgroundPlanets> planets,
                  ecs::Res<ecs::Time> time) {
    planets->countdown -= time->delta;
    if (planets.get().countdown > 0.0F) {
        return;
    }
    std::mt19937& engine = randomness->engine;
    planets->countdown = std::uniform_real_distribution<float>{kPlanetDelayMin, kPlanetDelayMax}(engine);
    const std::size_t index = std::uniform_int_distribution<std::size_t>{0, kPlanetImages.size() - 1}(engine);
    const float closeness = std::uniform_real_distribution<float>{0.0F, 1.0F}(engine);
    const float backgroundScale = kViewHeight / kBackgroundSize.y;
    const float scale = backgroundScale * std::lerp(kPlanetScaleMin, 1.0F, closeness);
    const float speed = backgroundScale * std::lerp(kPlanetSpeedMin, kPlanetSpeedMax, closeness);
    const glm::vec2 size = kPlanetSizes.at(index) * scale;
    commands.spawn()
        .add(plugins::transform::Transform{
            .position = {kViewWidth, std::uniform_real_distribution<float>{0.0F, kViewHeight - size.y}(engine)},
            .scale = glm::vec2{scale}})
        .add(plugins::backend::SpriteRenderer{.texture = planets.get().images.at(index),
                                              .source = {glm::vec2{0.0F}, kPlanetSizes.at(index)},
                                              .layer = kPlanetDrawLayer})
        .add(plugins::tilemap::Parallax{.scroll = {-speed, 0.0F}})
        .add(BackgroundPlanet{.width = size.x});
}

void removePlanets(
    ecs::Commands& commands,
    ecs::Query<const plugins::transform::Transform, const plugins::tilemap::Parallax, const BackgroundPlanet> planets) {
    for (auto [entity, transform, parallax, planet] : planets) {
        if (transform.position.x + parallax.scrolled.x + planet.width < 0.0F) {
            commands.despawn(entity);
        }
    }
}

}  // namespace rtype_duel
