/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioEvents
*/

#pragma once

#include <glm/ext/vector_float2.hpp>
#include <optional>

#include "plugins/AssetIdReflection.hpp"
#include "plugins/audio/AudioTypes.hpp"
#include "rtype/ecs/event/EventTraits.hpp"

namespace rtype::plugins::audio {
/// @brief A one-shot sound, fire and forget (a shot, an explosion). Dropped if its sound is not loaded by the time the
/// audio plugin plays it: a late shot sound is worse than a missing one.
struct PlaySound {
    core::asset::AssetId sound;         ///< The sound to play, from AudioAssets::load().
    Bus bus{Bus::kEffects};             ///< The bus it is mixed into.
    float volume{1.0F};                 ///< Linear gain, 1 = unchanged.
    float pitch{1.0F};                  ///< Playback speed, 1 = unchanged.
    std::optional<glm::vec2> position;  ///< World position to pan from; none: centred.
};
RTYPE_ECS_EVENT(PlaySound, "rtype.PlaySound");

/// @brief Replaces the music: the current track fades out while this one fades in, looping on the music bus. Waits for
/// its sound if it is not loaded yet; a later PlayMusic or StopMusic replaces it.
struct PlayMusic {
    core::asset::AssetId sound;  ///< The track, best loaded with SoundDesc::stream.
    float crossfade{1.0F};       ///< Seconds.
};
RTYPE_ECS_EVENT(PlayMusic, "rtype.PlayMusic", sound, crossfade);

/// @brief Fades the music out.
struct StopMusic {
    float fadeOut{1.0F};  ///< Seconds.
};
RTYPE_ECS_EVENT(StopMusic, "rtype.StopMusic", fadeOut);
}  // namespace rtype::plugins::audio
