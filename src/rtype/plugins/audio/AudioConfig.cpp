/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioConfig
*/

#include "plugins/audio/AudioConfig.hpp"

#include <cmath>
#include <cstdint>
#include <format>
#include <string_view>

#include "core/config/Settings.hpp"
#include "core/config/exceptions/ConfigExceptions.hpp"

namespace rtype::plugins::audio {
namespace {
/// @return The volume at @p key of the `audio.volume` section, or @p fallback.
/// @throws core::config::exceptions::SettingsException If it is negative.
float readVolume(const core::config::Settings& volume, std::string_view key, float fallback) {
    const double value = volume.getNumber(key, fallback);
    if (value < 0.0) {
        throw core::config::exceptions::SettingsException(
            std::format("Setting 'audio.volume.{}' must not be negative", key));
    }
    return static_cast<float>(value);
}
}  // namespace

AudioConfig AudioConfig::fromSettings(const core::config::Settings& settings) {
    settings.checkKeys({"backend", "required", "maxVoices", "volume", "panDistance"}, "audio");
    const core::config::Settings volume = settings.section("volume");
    volume.checkKeys({"master", "music", "effects"}, "audio.volume");

    AudioConfig config;
    config.backend = settings.getString("backend", config.backend);
    config.required = settings.getBool("required", config.required);
    const double maxVoices = settings.getNumber("maxVoices", config.maxVoices);
    if (maxVoices < 1.0 || std::floor(maxVoices) != maxVoices) {
        throw core::config::exceptions::SettingsException("Setting 'audio.maxVoices' must be a positive integer");
    }
    config.maxVoices = static_cast<std::uint32_t>(maxVoices);
    config.volume.master = readVolume(volume, "master", config.volume.master);
    config.volume.music = readVolume(volume, "music", config.volume.music);
    config.volume.effects = readVolume(volume, "effects", config.volume.effects);
    const double panDistance = settings.getNumber("panDistance", config.panDistance);
    if (panDistance <= 0.0) {
        throw core::config::exceptions::SettingsException("Setting 'audio.panDistance' must be positive");
    }
    config.panDistance = static_cast<float>(panDistance);
    return config;
}
}  // namespace rtype::plugins::audio
