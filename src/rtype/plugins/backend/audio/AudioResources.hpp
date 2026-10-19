/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioResources
*/

#pragma once

#include <cstdint>

namespace rtype::plugins::backend::audio {

/// @brief Refers to a sound effect of the audio backend, fully decoded in memory.
/// @details A plain value that owns nothing: the backend creates it (loadSound()) and destroys it (destroy(), or its
/// own destruction). A slot index plus the slot's generation, bumped when the slot is freed, so a stale id is detected.
struct SoundId {
    std::uint32_t index{0};       ///< Slot in the backend's table.
    std::uint32_t generation{0};  ///< Version of the slot; 0 means "no sound".

    [[nodiscard]] bool operator==(const SoundId& other) const = default;
};

/// @brief Refers to a music track of the audio backend, streamed from the disk while it plays.
/// @details Same rules as SoundId.
struct MusicId {
    std::uint32_t index{0};       ///< Slot in the backend's table.
    std::uint32_t generation{0};  ///< Version of the slot; 0 means "no music".

    [[nodiscard]] bool operator==(const MusicId& other) const = default;
};

/// @brief How one occurrence of a sound is played.
struct SoundParams {
    float volume = 1.0F;  ///< 0: silent, 1: as recorded.
    float pitch = 1.0F;   ///< Playback speed: 2 is one octave higher.
    float pan = 0.0F;     ///< -1: left, 0: centre, 1: right.
};
}  // namespace rtype::plugins::backend::audio
