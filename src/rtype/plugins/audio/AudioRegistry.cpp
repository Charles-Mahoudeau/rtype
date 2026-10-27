/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioRegistry
*/

#include "plugins/audio/AudioRegistry.hpp"

#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "plugins/audio/AudioConfig.hpp"
#include "plugins/audio/IAudioDevice.hpp"
#include "plugins/audio/NullAudioDevice.hpp"
#include "plugins/audio/exceptions/AudioExceptions.hpp"
#include "plugins/config/Settings.hpp"
#include "plugins/config/exceptions/ConfigExceptions.hpp"

namespace rtype::plugins::audio {

AudioRegistry::AudioRegistry() {
    add("null", [](const config::Settings& section, const AudioConfig& config) -> std::unique_ptr<IAudioDevice> {
        section.checkKeys({}, "null");
        return std::make_unique<NullAudioDevice>(config.maxVoices);
    });
}

void AudioRegistry::add(std::string name, Factory factory) {
    _factories.insert_or_assign(std::move(name), std::move(factory));
}

std::vector<std::string> AudioRegistry::getNames() const {
    std::vector<std::string> names;
    names.reserve(_factories.size());
    for (const auto& [name, factory] : _factories) {
        names.push_back(name);
    }
    return names;
}

std::unique_ptr<IAudioDevice> AudioRegistry::createDevice(const config::Settings& settings) const {
    const AudioConfig config = AudioConfig::fromSettings(settings.section("audio"));
    if (config.backend.empty()) {
        throw exceptions::AudioBackendException(
            R"(The config does not choose an audio backend ('audio.backend = "..."'))");
    }
    const auto found = _factories.find(config.backend);
    if (found == _factories.end()) {
        std::string available;
        for (const std::string& known : getNames()) {
            available += (available.empty() ? "" : ", ") + known;
        }
        throw exceptions::AudioBackendException(
            std::format("Unknown audio backend '{}' (registered: {})", config.backend, available));
    }
    std::string failure;
    try {
        std::unique_ptr<IAudioDevice> device = found->second(settings.section(config.backend), config);
        if (device) {
            return device;
        }
        failure = "its factory returned no device";
    } catch (const config::exceptions::SettingsException&) {
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
