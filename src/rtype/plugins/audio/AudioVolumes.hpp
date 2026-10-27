/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioVolumes
*/

#pragma once

#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::audio {
/// @brief The master and bus volumes, as the ECS resource "rtype.AudioVolumes": linear gains, 1 = unchanged.
/// @details Starts from the `audio.volume` section of the config when the device is created; a settings menu writes
/// it, and the audio plugin applies it to the device in the frame it changed.
struct AudioVolumes {
    float master{1.0F};   ///< Applied to everything, on top of the bus volumes.
    float music{1.0F};    ///< The Bus::kMusic bus.
    float effects{1.0F};  ///< The Bus::kEffects bus.
};
RTYPE_ECS_RESOURCE(AudioVolumes, "rtype.AudioVolumes", master, music, effects);
}  // namespace rtype::plugins::audio
