/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FakeAudioDevice
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <map>
#include <memory>
#include <set>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "core/config/Settings.hpp"
#include "plugins/audio/AudioConfig.hpp"
#include "plugins/audio/AudioRegistry.hpp"
#include "plugins/audio/AudioTypes.hpp"
#include "plugins/audio/IAudioDevice.hpp"
#include "plugins/audio/exceptions/AudioExceptions.hpp"

namespace rtype::test::plugins {
/// @brief One play() call.
struct FakePlay {
    rtype::plugins::audio::SoundId sound;      ///< The sound played.
    rtype::plugins::audio::PlayParams params;  ///< How it started.
    rtype::plugins::audio::VoiceId voice;      ///< The voice handed out.
};

/// @brief One createSound() call.
struct FakeSound {
    std::string path;                       ///< The path, for a sound loaded from a file; empty otherwise.
    std::size_t byteCount{0};               ///< The bytes given, for a sound loaded from memory.
    rtype::plugins::audio::SoundDesc desc;  ///< How it is loaded.
};

/// @brief What the fake device does and records, shared with the test that drives it.
struct FakeAudioState {
    std::set<std::string> failingPaths;                     ///< Paths createSound() throws for.
    bool failToOpen{false};                                 ///< The registry's factory throws.
    std::vector<FakeSound> sounds;                          ///< Every createSound(), by SoundId - 1.
    std::vector<rtype::plugins::audio::SoundId> destroyed;  ///< Every destroySound().
    std::vector<FakePlay> plays;                            ///< Every play(), by VoiceId::index.
    std::vector<std::pair<rtype::plugins::audio::VoiceId, float>>
        stops;                                               ///< Every stop() of a live voice, and its fade.
    std::map<std::uint32_t, float> volumes;                  ///< The last setVolume() of each voice index.
    std::map<std::uint32_t, float> pitches;                  ///< The last setPitch() of each voice index.
    std::map<std::uint32_t, float> pans;                     ///< The last setPan() of each voice index.
    float masterVolume{-1.0F};                               ///< The last setMasterVolume().
    std::map<rtype::plugins::audio::Bus, float> busVolumes;  ///< The last setBusVolume() of each bus.
    std::set<std::uint32_t> playing;                         ///< The voice indices playing.
    int updates{0};                                          ///< update() calls.
    int stopCalls{0};                                        ///< stop() calls, on live or stale voices.
    int masterVolumeCalls{0};                                ///< setMasterVolume() calls.
};

/// @brief An audio device that plays nothing and records every call. Voices play until stopped or ended by the test
/// (FakeAudioState::playing).
class FakeAudioDevice : public rtype::plugins::audio::IAudioDevice {
  public:
    explicit FakeAudioDevice(std::shared_ptr<FakeAudioState> state) : _state{std::move(state)} {}
    ~FakeAudioDevice() override = default;
    FakeAudioDevice(const FakeAudioDevice& other) = delete;
    FakeAudioDevice& operator=(const FakeAudioDevice& other) = delete;
    FakeAudioDevice(FakeAudioDevice&& other) = delete;
    FakeAudioDevice& operator=(FakeAudioDevice&& other) = delete;

    rtype::plugins::audio::SoundId createSound(std::span<const std::byte> encoded,
                                               const rtype::plugins::audio::SoundDesc& desc) override {
        _state->sounds.push_back(FakeSound{.byteCount = encoded.size(), .desc = desc});
        return {.value = static_cast<std::uint32_t>(_state->sounds.size())};
    }

    rtype::plugins::audio::SoundId createSound(const std::filesystem::path& path,
                                               const rtype::plugins::audio::SoundDesc& desc) override {
        if (_state->failingPaths.contains(path.generic_string())) {
            throw rtype::plugins::audio::exceptions::AudioLoadException{
                std::format("Sound '{}': cannot open the file", path.generic_string())};
        }
        _state->sounds.push_back(FakeSound{.path = path.generic_string(), .desc = desc});
        return {.value = static_cast<std::uint32_t>(_state->sounds.size())};
    }

    void destroySound(rtype::plugins::audio::SoundId sound) override { _state->destroyed.push_back(sound); }

    rtype::plugins::audio::VoiceId play(rtype::plugins::audio::SoundId sound,
                                        const rtype::plugins::audio::PlayParams& params) override {
        const rtype::plugins::audio::VoiceId voice{.index = static_cast<std::uint32_t>(_state->plays.size()),
                                                   .generation = 1};
        _state->plays.push_back(FakePlay{.sound = sound, .params = params, .voice = voice});
        _state->playing.insert(voice.index);
        return voice;
    }

    void stop(rtype::plugins::audio::VoiceId voice, float fadeOut) override {
        ++_state->stopCalls;
        if (isPlaying(voice)) {
            _state->stops.emplace_back(voice, fadeOut);
            _state->playing.erase(voice.index);
        }
    }

    void setPaused(rtype::plugins::audio::VoiceId /*voice*/, bool /*paused*/) override {}

    void setVolume(rtype::plugins::audio::VoiceId voice, float volume) override {
        if (isPlaying(voice)) {
            _state->volumes[voice.index] = volume;
        }
    }

    void setPitch(rtype::plugins::audio::VoiceId voice, float pitch) override {
        if (isPlaying(voice)) {
            _state->pitches[voice.index] = pitch;
        }
    }

    void setPan(rtype::plugins::audio::VoiceId voice, float pan) override {
        if (isPlaying(voice)) {
            _state->pans[voice.index] = pan;
        }
    }

    [[nodiscard]] bool isPlaying(rtype::plugins::audio::VoiceId voice) const override {
        return voice.generation == 1 && _state->playing.contains(voice.index);
    }

    void setMasterVolume(float volume) override {
        _state->masterVolume = volume;
        ++_state->masterVolumeCalls;
    }

    void setBusVolume(rtype::plugins::audio::Bus bus, float volume) override { _state->busVolumes[bus] = volume; }

    void update() override { ++_state->updates; }

  private:
    std::shared_ptr<FakeAudioState> _state;  ///< Shared with the test.
};

/// @return A registry with the fake device registered as "fake", besides "null".
inline std::unique_ptr<rtype::plugins::audio::AudioRegistry> makeFakeAudioRegistry(
    const std::shared_ptr<FakeAudioState>& state) {
    auto registry = std::make_unique<rtype::plugins::audio::AudioRegistry>();
    registry->add(
        "fake",
        [state](const rtype::core::config::Settings& section, const rtype::plugins::audio::AudioConfig& /*config*/)
            -> std::unique_ptr<rtype::plugins::audio::IAudioDevice> {
            section.checkKeys({}, "fake");
            if (state->failToOpen) {
                throw rtype::plugins::audio::exceptions::AudioLoadException{"no output device"};
            }
            return std::make_unique<FakeAudioDevice>(state);
        });
    return registry;
}

/// @return A configuration naming the fake device.
inline rtype::core::config::Settings makeFakeAudioSettings() {
    rtype::core::config::Settings settings;
    settings.set("audio.backend", "fake");
    return settings;
}
}  // namespace rtype::test::plugins
