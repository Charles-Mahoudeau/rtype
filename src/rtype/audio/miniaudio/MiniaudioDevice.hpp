/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MiniaudioDevice
*/

#pragma once

#include <miniaudio.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "audio/miniaudio/MiniaudioConfig.hpp"
#include "audio/miniaudio/VoicePool.hpp"
#include "audio/miniaudio/core/Engine.hpp"
#include "audio/miniaudio/core/Sound.hpp"
#include "audio/miniaudio/core/SoundGroup.hpp"
#include "plugins/audio/AudioConfig.hpp"
#include "plugins/audio/AudioTypes.hpp"
#include "plugins/audio/IAudioDevice.hpp"

namespace rtype::audio::miniaudio {
/// @brief The miniaudio backend: plays sounds through miniaudio's engine, with one sound group per bus.
/// @details Internal to this module, as it includes miniaudio.h: the executable only sees registerAudio()
/// (Registration.hpp). Called from the main thread only, as every IAudioDevice; miniaudio mixes on its own audio
/// thread. miniaudio keeps pointers between its objects (a sound to its group, a group to the engine), so they must
/// never move: the device is neither copyable nor movable, and lives behind the registry's std::unique_ptr.
class MiniaudioDevice : public plugins::audio::IAudioDevice {
  public:
    /// @param config The common `audio` section (maxVoices...).
    /// @param miniaudio The `miniaudio` section: how the engine mixes.
    /// @throws exceptions::MiniaudioException If the device cannot open.
    MiniaudioDevice(const plugins::audio::AudioConfig& config, const MiniaudioConfig& miniaudio);
    ~MiniaudioDevice() override;
    MiniaudioDevice(const MiniaudioDevice& other) = delete;
    MiniaudioDevice& operator=(const MiniaudioDevice& other) = delete;
    MiniaudioDevice(MiniaudioDevice&& other) = delete;
    MiniaudioDevice& operator=(MiniaudioDevice&& other) = delete;

    plugins::audio::SoundId createSound(std::span<const std::byte> encoded,
                                        const plugins::audio::SoundDesc& desc) override;
    plugins::audio::SoundId createSound(const std::filesystem::path& path,
                                        const plugins::audio::SoundDesc& desc) override;
    void destroySound(plugins::audio::SoundId sound) override;

    plugins::audio::VoiceId play(plugins::audio::SoundId sound, const plugins::audio::PlayParams& params) override;
    void stop(plugins::audio::VoiceId voice, float fadeOut) override;
    void setPaused(plugins::audio::VoiceId voice, bool paused) override;
    void setVolume(plugins::audio::VoiceId voice, float volume) override;
    void setPitch(plugins::audio::VoiceId voice, float pitch) override;
    void setPan(plugins::audio::VoiceId voice, float pan) override;
    [[nodiscard]] bool isPlaying(plugins::audio::VoiceId voice) const override;

    void setMasterVolume(float volume) override;
    void setBusVolume(plugins::audio::Bus bus, float volume) override;

    void update() override;

  private:
    /// @brief A sound createSound() loaded: the model play() copies, and what is needed to open a streamed one again.
    struct LoadedSound {
        std::string name;                    ///< The file path, or the name its bytes are registered under.
        plugins::audio::SoundDesc desc;      ///< Streamed or decoded.
        std::vector<std::byte> encoded;      ///< The bytes given in memory, registered under `name`; empty for a file.
        std::unique_ptr<core::Sound> model;  ///< Loaded once to check the file and fill the cache; never played.
    };

    /// @return The group of @p bus.
    [[nodiscard]] core::SoundGroup& groupOf(plugins::audio::Bus bus);

    /// @brief Loads @p sound's model and records it under a new id.
    /// @throws exceptions::MiniaudioException If it cannot be opened or decoded.
    plugins::audio::SoundId addSound(LoadedSound sound);

    /// @brief Frees @p sound's model, then unregisters its bytes, if it has any.
    void releaseSound(LoadedSound& sound) noexcept;

    /// @return The MA_SOUND_FLAG_* bits a sound loaded with @p desc is opened with.
    [[nodiscard]] static ma_uint32 flagsOf(const plugins::audio::SoundDesc& desc) noexcept;

    core::Engine _engine;  ///< The device and the mixer; declared first, so destroyed after what is attached to it.
    std::array<core::SoundGroup, 2> _buses;                  ///< One per Bus, indexed by it: kMusic, kEffects.
    std::unordered_map<std::uint32_t, LoadedSound> _sounds;  ///< The sounds loaded, by SoundId::value.
    std::uint32_t _nextSound{1};                             ///< The id of the next sound; 0 means "no sound".
    VoicePool _voices;  ///< The voices playing; declared last, so destroyed before the groups and sounds they use.
};
}  // namespace rtype::audio::miniaudio
