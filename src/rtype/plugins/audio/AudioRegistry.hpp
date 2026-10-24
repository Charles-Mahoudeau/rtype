/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioRegistry
*/

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "plugins/audio/AudioConfig.hpp"
#include "plugins/audio/IAudioDevice.hpp"
#include "plugins/config/Settings.hpp"

namespace rtype::plugins::audio {
/// @brief Audio backends by name, so the configuration picks one instead of the code.
///
/// @details Each module registers its backend (rtype::audio::miniaudio::registerAudio()...), then createDevice() starts
/// the one the config names:
/// @code{.lua}
/// return {
///     audio = { backend = "miniaudio", volume = { music = 0.7 } },  -- for every backend
///     miniaudio = { sampleRate = 48000 },                           -- only for the "miniaudio" backend
/// }
/// @endcode
/// Each factory receives the section named after it, which it checks and turns into its own typed configuration, and
/// the common `audio` section, already typed. "null" is always registered.
class AudioRegistry {
  public:
    /// @param section The backend's own section (`miniaudio`...), to check and read.
    /// @param config The common `audio` section, already typed.
    /// @throws Anything if the device cannot open: createDevice() falls back to "null", or rethrows if required.
    using Factory =
        std::function<std::unique_ptr<IAudioDevice>(const config::Settings& section, const AudioConfig& config)>;

    /// @brief Registers "null" (NullAudioDevice).
    AudioRegistry();
    ~AudioRegistry() = default;
    AudioRegistry(const AudioRegistry& other) = delete;
    AudioRegistry& operator=(const AudioRegistry& other) = delete;
    AudioRegistry(AudioRegistry&& other) = delete;
    AudioRegistry& operator=(AudioRegistry&& other) = delete;

    /// @brief Registers a backend under a name, replacing any previous one with the same name.
    void add(std::string name, Factory factory);

    /// @return The registered names, sorted.
    [[nodiscard]] std::vector<std::string> getNames() const;

    /// @brief Creates the backend `audio.backend` names, with its section of @p settings.
    /// @param settings The whole configuration: `audio`, and a section per backend.
    /// @return The device; a NullAudioDevice, with a warning, if the backend cannot open and `audio.required` is false.
    /// @throws exceptions::AudioBackendException If the name is missing or not registered, or if the device cannot
    /// open and `audio.required` is true.
    /// @throws config::exceptions::SettingsException If a setting is unknown or has the wrong type.
    [[nodiscard]] std::unique_ptr<IAudioDevice> createDevice(const config::Settings& settings) const;

  private:
    std::map<std::string, Factory, std::less<>> _factories;  ///< Factories by name.
};
}  // namespace rtype::plugins::audio
