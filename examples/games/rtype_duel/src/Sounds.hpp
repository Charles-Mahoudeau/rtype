/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Sounds
*/

#pragma once

#include "Components.hpp"
#include "Resources.hpp"
#include "plugins/audio/AudioAssets.hpp"
#include "plugins/audio/AudioEvents.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

// Client systems: the music and the sounds of the shots.

namespace rtype_duel {

/// @brief Startup: asks for the sounds, and starts the music, which loops once it is loaded.
void loadSounds(rtype::ecs::ResMut<rtype::plugins::audio::AudioAssets> sounds, rtype::ecs::ResMut<GameSounds> game,
                rtype::ecs::EventWriter<rtype::plugins::audio::PlayMusic> music);

/// @brief Update: plays a sound for every laser and misfire the server just created, panned where it happened, at a
/// random pitch and volume, so that no two shots sound the same.
void playShots(rtype::ecs::ResMut<Randomness> randomness, rtype::ecs::Res<GameSounds> game,
               rtype::ecs::EventWriter<rtype::plugins::audio::PlaySound> sounds,
               rtype::ecs::Query<const Laser, rtype::ecs::Added<Laser>> lasers,
               rtype::ecs::Query<const Misfire, rtype::ecs::Added<Misfire>> misfires);

}  // namespace rtype_duel
