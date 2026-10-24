/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Registration
*/

#pragma once

#include "plugins/audio/AudioRegistry.hpp"

namespace rtype::audio::miniaudio {

/// @brief Registers the audio backends of this module: "miniaudio" (MiniaudioDevice), configured by the `miniaudio`
/// section (MiniaudioConfig).
void registerAudio(plugins::audio::AudioRegistry& registry);

}  // namespace rtype::audio::miniaudio
