/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MiniaudioConfig
*/

#include "audio/miniaudio/MiniaudioConfig.hpp"

#include <cmath>
#include <cstdint>
#include <format>
#include <limits>
#include <string_view>

#include "plugins/config/Settings.hpp"
#include "plugins/config/exceptions/ConfigExceptions.hpp"

namespace rtype::audio::miniaudio {
namespace {
/// @return The value at @p key of the `miniaudio` section, or @p fallback.
/// @throws plugins::config::exceptions::SettingsException If it is not a non-negative integer.
std::uint32_t readCount(const plugins::config::Settings& settings, std::string_view key, std::uint32_t fallback) {
    const double value = settings.getNumber(key, fallback);
    if (value < 0.0 || std::floor(value) != value || value > std::numeric_limits<std::uint32_t>::max()) {
        throw plugins::config::exceptions::SettingsException(
            std::format("Setting 'miniaudio.{}' must be a non-negative integer", key));
    }
    return static_cast<std::uint32_t>(value);
}
}  // namespace

MiniaudioConfig MiniaudioConfig::fromSettings(const plugins::config::Settings& settings) {
    settings.checkKeys({"sampleRate", "channels", "periodSizeInMilliseconds"}, "miniaudio");
    MiniaudioConfig config;
    config.sampleRate = readCount(settings, "sampleRate", config.sampleRate);
    config.channels = readCount(settings, "channels", config.channels);
    config.periodSizeInMilliseconds = readCount(settings, "periodSizeInMilliseconds", config.periodSizeInMilliseconds);
    return config;
}
}  // namespace rtype::audio::miniaudio
