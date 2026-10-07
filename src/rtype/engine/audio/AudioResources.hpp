/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioResources
*/

#pragma once

#include "engine/resource/Handle.hpp"
#include "engine/resource/Resource.hpp"

namespace rtype::engine::audio {

class IAudio;

struct SoundTag {};
struct MusicTag {};

/// @name Handles: refer to an audio resource without owning it.
/// @{
using SoundId = resource::Handle<SoundTag>;
using MusicId = resource::Handle<MusicTag>;
/// @}

/// @name Owners: destroy the audio resource when going out of scope.
/// @{
using Sound = resource::Resource<SoundTag, IAudio>;  ///< Short effect, fully decoded in memory.
using Music = resource::Resource<MusicTag, IAudio>;  ///< Long track, streamed from the disk.
/// @}

/// @brief How one occurrence of a sound is played.
struct SoundParams {
    float volume = 1.0F;  ///< 0: silent, 1: as recorded.
    float pitch = 1.0F;   ///< Playback speed: 2 is one octave higher.
    float pan = 0.0F;     ///< -1: left, 0: centre, 1: right.
};
}  // namespace rtype::engine::audio
