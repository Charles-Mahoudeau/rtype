/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** IAudio
*/

#pragma once

#include <filesystem>

#include "engine/audio/AudioResources.hpp"

namespace rtype::engine::audio {

/// @brief Sound effects and music, implemented by an audio backend (none yet).
///
/// @details Independent from the window and the renderer. Loading returns a handle the caller wraps in its owner:
/// @code
/// const Sound laser(audio, audio.loadSound("assets/laser.wav"));
/// audio.play(laser.getId(), {.volume = 0.5F});
/// @endcode
class IAudio {
  public:
    IAudio() = default;
    virtual ~IAudio() = default;
    IAudio(const IAudio&) = delete;
    IAudio& operator=(const IAudio&) = delete;
    IAudio(IAudio&&) = delete;
    IAudio& operator=(IAudio&&) = delete;

    /// @return The volume applied to everything, in [0, 1].
    [[nodiscard]] virtual float getMasterVolume() const = 0;

    /// @brief Sets the volume applied to everything, in [0, 1].
    virtual void setMasterVolume(float volume) = 0;

    /// @name Resources
    /// @{

    /// @brief Decodes a whole sound file in memory: for short, frequent effects.
    [[nodiscard]] virtual SoundId loadSound(const std::filesystem::path& path) = 0;

    /// @brief Opens a music file, streamed while it plays: for long tracks.
    [[nodiscard]] virtual MusicId loadMusic(const std::filesystem::path& path) = 0;

    virtual void destroy(SoundId sound) = 0;
    virtual void destroy(MusicId music) = 0;
    /// @}

    /// @name Playback
    /// @{

    /// @brief Plays the sound once. Fire and forget: the same sound may overlap itself.
    virtual void play(SoundId sound, const SoundParams& params) = 0;

    /// @brief Starts or resumes a music.
    virtual void play(MusicId music, bool loop) = 0;
    virtual void pause(MusicId music) = 0;
    /// @brief Stops a music and rewinds it.
    virtual void stop(MusicId music) = 0;
    /// @}

    /// @brief Refills the music streams and releases finished sounds. Call once per frame.
    virtual void update() = 0;
};
}  // namespace rtype::engine::audio
