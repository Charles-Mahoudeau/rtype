/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioRegistry
*/

#include "plugins/audio/AudioRegistry.hpp"

#include <cstdint>
#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <string>

#include "core/config/Settings.hpp"
#include "core/config/exceptions/ConfigExceptions.hpp"
#include "plugins/audio/AudioConfig.hpp"
#include "plugins/audio/NullAudioDevice.hpp"
#include "rtype/contract/audiodevice/AudioDeviceRegistry.hpp"
#include "rtype/contract/audiodevice/IAudioDevice.hpp"
#include "rtype/contract/audiodevice/exceptions/AudioExceptions.hpp"

namespace rtype::plugins::audio {

namespace exceptions = contract::audiodevice::exceptions;

AudioRegistry::AudioRegistry() {
    devices.add("null",
                [](const core::config::Settings& section,
                   std::uint32_t maxVoices) -> std::unique_ptr<contract::audiodevice::IAudioDevice> {
                    section.checkKeys({}, "null");
                    return std::make_unique<NullAudioDevice>(maxVoices);
                });
}

std::unique_ptr<contract::audiodevice::IAudioDevice> AudioRegistry::createDevice(
    const core::config::Settings& settings) const {
    const AudioConfig config = AudioConfig::fromSettings(settings.section("audio"));
    if (config.backend.empty()) {
        throw exceptions::AudioBackendException(
            R"(The config does not choose an audio backend ('audio.backend = "..."'))");
    }
    const contract::audiodevice::AudioDeviceRegistry::Factory* factory = devices.find(config.backend);
    if (factory == nullptr) {
        std::string available;
        for (const std::string& known : devices.getNames()) {
            available += (available.empty() ? "" : ", ") + known;
        }
        throw exceptions::AudioBackendException(
            std::format("Unknown audio backend '{}' (registered: {})", config.backend, available));
    }
    std::string failure;
    try {
        std::unique_ptr<contract::audiodevice::IAudioDevice> device =
            (*factory)(settings.section(config.backend), config.maxVoices);
        if (device) {
            return device;
        }
        failure = "its factory returned no device";
    } catch (const core::config::exceptions::SettingsException&) {
        throw;
    } catch (const std::exception& error) {
        failure = error.what();
    }
    if (config.required) {
        throw exceptions::AudioBackendException(
            std::format("The audio backend '{}' cannot start: {}", config.backend, failure));
    }
    std::cerr << std::format("[AudioRegistry] The audio backend '{}' cannot start ({}): playing no sound\n",
                             config.backend, failure);
    return std::make_unique<NullAudioDevice>(config.maxVoices);
}
}  // namespace rtype::plugins::audio
