/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioRegistry
*/

#pragma once

#include <memory>

#include "core/config/Settings.hpp"
#include "plugins/audio/Export.hpp"
#include "rtype/contract/audiodevice/AudioDeviceRegistry.hpp"
#include "rtype/contract/audiodevice/IAudioDevice.hpp"

namespace rtype::plugins::audio {
/// @brief The audio devices a program can start, so the configuration picks one instead of the code.
///
/// @details Each module registers its device (rtype::audio::miniaudio::registerAudio()...), then createDevice() starts
/// the one the config names:
/// @code{.lua}
/// return {
///     audio = { backend = "miniaudio", volume = { music = 0.7 } },  -- for every backend
///     miniaudio = { sampleRate = 48000 },                           -- only for the "miniaudio" backend
/// }
/// @endcode
/// "null" is always registered.
struct RTYPE_PLUGIN_AUDIO_API AudioRegistry {
    contract::audiodevice::AudioDeviceRegistry devices;  ///< The devices by name.

    /// @brief Registers "null" (NullAudioDevice).
    AudioRegistry();
    ~AudioRegistry() = default;
    AudioRegistry(const AudioRegistry& other) = delete;
    AudioRegistry& operator=(const AudioRegistry& other) = delete;
    AudioRegistry(AudioRegistry&& other) = delete;
    AudioRegistry& operator=(AudioRegistry&& other) = delete;

    /// @brief Creates the device `audio.backend` names, with its section of @p settings.
    /// @param settings The whole configuration: `audio`, and a section per device.
    /// @return The device; a NullAudioDevice, with a warning, if it cannot open and `audio.required` is false.
    /// @throws contract::audiodevice::exceptions::AudioBackendException If the name is missing or not registered, or if
    /// the device cannot open and `audio.required` is true.
    /// @throws core::config::exceptions::SettingsException If a setting is unknown or has the wrong type.
    [[nodiscard]] std::unique_ptr<contract::audiodevice::IAudioDevice> createDevice(
        const core::config::Settings& settings) const;
};
}  // namespace rtype::plugins::audio
