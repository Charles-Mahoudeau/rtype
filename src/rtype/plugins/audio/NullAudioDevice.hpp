/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NullAudioDevice
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <unordered_set>
#include <vector>

#include "plugins/audio/AudioTypes.hpp"
#include "plugins/audio/IAudioDevice.hpp"

namespace rtype::plugins::audio {
/// @brief The silent backend, registered as "null" by every AudioRegistry: plays nothing, but keeps the state of its
/// sounds and voices, so the audio plugin runs the same with it (tests, CI, a machine without an output, sound off).
/// @details A looping voice plays until stopped; a one-shot voice ends at the next update(), as if it were one frame
/// long. Nothing is read or decoded: every sound loads.
class NullAudioDevice : public IAudioDevice {
  public:
    /// @param maxVoices Voices playing at once; play() returns a stale id beyond that.
    explicit NullAudioDevice(std::uint32_t maxVoices = 64);
    ~NullAudioDevice() override = default;
    NullAudioDevice(const NullAudioDevice& other) = delete;
    NullAudioDevice& operator=(const NullAudioDevice& other) = delete;
    NullAudioDevice(NullAudioDevice&& other) = delete;
    NullAudioDevice& operator=(NullAudioDevice&& other) = delete;

    SoundId createSound(std::span<const std::byte> encoded, const SoundDesc& desc) override;
    SoundId createSound(const std::filesystem::path& path, const SoundDesc& desc) override;
    void destroySound(SoundId sound) override;

    VoiceId play(SoundId sound, const PlayParams& params) override;
    void stop(VoiceId voice, float fadeOut) override;
    void setPaused(VoiceId voice, bool paused) override;
    void setVolume(VoiceId voice, float volume) override;
    void setPitch(VoiceId voice, float pitch) override;
    void setPan(VoiceId voice, float pan) override;
    [[nodiscard]] bool isPlaying(VoiceId voice) const override;

    void setMasterVolume(float volume) override;
    void setBusVolume(Bus bus, float volume) override;

    void update() override;

  private:
    /// @brief One slot of the voice table.
    struct Voice {
        std::uint32_t generation{0};  ///< Of the voice in the slot; bumped when the slot is reused.
        SoundId sound;                ///< The sound it plays.
        bool playing{false};          ///< False once it ended or was stopped: the slot is free.
        bool loop{false};             ///< Plays until stopped.
        bool paused{false};           ///< Does not end while paused.
    };

    /// @return The voice @p voice names, or nullptr if it is stale.
    [[nodiscard]] Voice* find(VoiceId voice);

    /// @return The voice @p voice names, or nullptr if it is stale.
    [[nodiscard]] const Voice* find(VoiceId voice) const;

    std::uint32_t _maxVoices;                   ///< Size limit of _voices.
    std::uint32_t _nextSound{1};                ///< The id of the next sound created.
    std::unordered_set<std::uint32_t> _sounds;  ///< The ids of the sounds alive.
    std::vector<Voice> _voices;                 ///< The voice table, indexed by VoiceId::index.
};
}  // namespace rtype::plugins::audio
