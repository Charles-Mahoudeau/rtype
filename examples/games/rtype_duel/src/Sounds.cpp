/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Sounds
*/

#include "Sounds.hpp"

#include <random>

#include "Constants.hpp"
#include "plugins/audio/AudioTypes.hpp"

using namespace rtype;

namespace rtype_duel {

void loadSounds(ecs::ResMut<plugins::audio::AudioAssets> sounds, ecs::ResMut<GameSounds> game,
                ecs::EventWriter<plugins::audio::PlayMusic> music) {
    game->music = sounds->load(kMusicSound, plugins::audio::SoundDesc{.stream = true});
    game->laser = sounds->load(kLaserSound);
    game->misfire = sounds->load(kMisfireSound);
    music.send(plugins::audio::PlayMusic{.sound = game->music, .crossfade = 1.0F});
}

void playShots(ecs::ResMut<Randomness> randomness, ecs::Res<GameSounds> game,
               ecs::EventWriter<plugins::audio::PlaySound> sounds, ecs::Query<const Laser, ecs::Added<Laser>> lasers,
               ecs::Query<const Misfire, ecs::Added<Misfire>> misfires) {
    std::uniform_real_distribution<float> pitch{kShotPitchMin, kShotPitchMax};
    std::uniform_real_distribution<float> volume{kShotVolumeMin, kShotVolumeMax};
    std::mt19937& engine = randomness->engine;
    for (auto [entity, laser] : lasers) {
        sounds.send(plugins::audio::PlaySound{
            .sound = game->laser, .volume = volume(engine), .pitch = pitch(engine), .position = laser.position});
    }
    for (auto [entity, misfire] : misfires) {
        sounds.send(plugins::audio::PlaySound{
            .sound = game->misfire, .volume = volume(engine), .pitch = pitch(engine), .position = misfire.position});
    }
}

}  // namespace rtype_duel
