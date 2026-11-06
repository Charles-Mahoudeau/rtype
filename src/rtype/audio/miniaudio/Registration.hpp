/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Registration
*/

#pragma once

#include "rtype/audio/miniaudio/Export.hpp"
#include "rtype/contract/audiodevice/AudioDeviceRegistry.hpp"

namespace rtype::audio::miniaudio {

/// @brief Registers the audio backends of this module: "miniaudio" (MiniaudioDevice), configured by the `miniaudio`
/// section (MiniaudioConfig).
RTYPE_AUDIO_MINIAUDIO_API void registerAudio(contract::audiodevice::AudioDeviceRegistry& registry);

}  // namespace rtype::audio::miniaudio
