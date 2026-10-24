/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** IAudioDevice
*/

#pragma once

#include <cstddef>
#include <filesystem>
#include <span>

#include "plugins/Export.hpp"
#include "plugins/audio/AudioTypes.hpp"

namespace rtype::plugins::audio {
/// @brief An audio output: the contract every audio backend implements. Called from the main thread only.
class RTYPE_PLUGINS_API IAudioDevice {
  public:
    IAudioDevice() = default;
    virtual ~IAudioDevice() = default;
    IAudioDevice(const IAudioDevice&) = delete;
    IAudioDevice& operator=(const IAudioDevice&) = delete;
    IAudioDevice(IAudioDevice&&) = delete;
    IAudioDevice& operator=(IAudioDevice&&) = delete;

    /// @name Sounds
    /// @{
    /// @brief Loads a sound from an encoded file in memory (WAV, OGG, MP3, FLAC). The backend decodes it.
    /// @throws exceptions::AudioLoadException If the data cannot be decoded.
    virtual SoundId createSound(std::span<const std::byte> encoded, const SoundDesc& desc) = 0;
    /// @brief Loads a sound from a file; a streamed sound keeps reading it while it plays.
    virtual SoundId createSound(const std::filesystem::path& path, const SoundDesc& desc) = 0;
    /// @brief Stops every voice of the sound and frees it.
    virtual void destroySound(SoundId sound) = 0;
    /// @}

    /// @name Voices
    /// @{
    /// @return The new voice, or a stale id if no voice could be found (maxVoices reached, nothing to steal).
    virtual VoiceId play(SoundId sound, const PlayParams& params) = 0;
    /// @brief Stops the voice, fading out over @p fadeOut seconds.
    virtual void stop(VoiceId voice, float fadeOut = 0.0F) = 0;
    virtual void setPaused(VoiceId voice, bool paused) = 0;
    virtual void setVolume(VoiceId voice, float volume) = 0;
    virtual void setPitch(VoiceId voice, float pitch) = 0;
    virtual void setPan(VoiceId voice, float pan) = 0;
    /// @return False once the voice has ended or been stopped.
    [[nodiscard]] virtual bool isPlaying(VoiceId voice) const = 0;
    /// @}

    /// @name Mixing
    /// @{
    virtual void setMasterVolume(float volume) = 0;
    virtual void setBusVolume(Bus bus, float volume) = 0;
    /// @}

    /// @brief Called once per frame: reclaims the ended voices (miniaudio), or runs the library's update (FMOD's
    /// System::update()).
    virtual void update() = 0;
};
}  // namespace rtype::plugins::audio
