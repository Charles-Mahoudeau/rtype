/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioCalls
*/

#pragma once

#include <glm/ext/vector_float2.hpp>
#include <limits>
#include <string_view>

#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "plugins/GlmReflection.hpp"      // IWYU pragma: keep
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/call/CallTraits.hpp"
#include "rtype/ecs/type/String.hpp"

namespace rtype::plugins::audio {

inline constexpr std::string_view kLoadSoundCall = "rtype.audio.loadSound";  ///< AudioAssets::load().
inline constexpr std::string_view kPlaySoundCall = "rtype.audio.playSound";  ///< Sends a PlaySound.

/// @brief The arguments of `rtype.audio.loadSound`: asks for a sound file.
struct LoadSoundArguments {
    ecs::String path;    ///< The sound file (WAV, OGG, MP3, FLAC).
    bool stream{false};  ///< Decoded while it plays (music), instead of decoded once into memory (effects).
};
RTYPE_ECS_CALL_DATA(LoadSoundArguments, "rtype.audio.LoadSoundArguments", path, stream);

/// @brief The result of `rtype.audio.loadSound`.
struct SoundResult {
    core::asset::AssetId sound;  ///< The sound's id, for `rtype.audio.playSound`, PlayMusic and AudioSource.
};
RTYPE_ECS_CALL_DATA(SoundResult, "rtype.audio.SoundResult", sound);

/// @brief The arguments of `rtype.audio.playSound`: a PlaySound, which scripts cannot send as an event (its position
/// is optional).
struct PlaySoundArguments {
    core::asset::AssetId sound;  ///< The sound, from `rtype.audio.loadSound` (or its path).
    ecs::String bus{"effects"};  ///< "effects" or "music".
    float volume{1.0F};          ///< Linear gain, 1 = unchanged.
    float pitch{1.0F};           ///< Playback speed, 1 = unchanged.
    /// @brief The world position to pan from; NaN (not given): centred.
    glm::vec2 position{std::numeric_limits<float>::quiet_NaN()};
};
RTYPE_ECS_CALL_DATA(PlaySoundArguments, "rtype.audio.PlaySoundArguments", sound, bus, volume, pitch, position);

/// @brief Registers the audio engine calls on @p app, for scripts: `rtype.audio.loadSound`, on the
/// `rtype.AudioAssets` resource, and `rtype.audio.playSound`, which sends a PlaySound (an unknown bus throws an
/// AudioLoadException). Called by the AudioPlugin.
void addAudioCalls(ecs::App& app);

}  // namespace rtype::plugins::audio
