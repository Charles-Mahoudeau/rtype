/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MiniaudioConfig
*/

#pragma once

#include <cstdint>

#include "core/config/Settings.hpp"

namespace rtype::audio::miniaudio {
/// @brief The settings of the "miniaudio" backend, read from the `miniaudio` section of the config (fromSettings()).
/// Every field has a default: 0 lets miniaudio choose, from the output device.
/// @code{.lua}
/// miniaudio = { sampleRate = 48000, channels = 2, periodSizeInMilliseconds = 10 },
/// @endcode
struct MiniaudioConfig {
    std::uint32_t sampleRate{0};                ///< Mixing sample rate in Hz; 0: the device's own.
    std::uint32_t channels{0};                  ///< Mixing channels (2: stereo); 0: the device's own.
    std::uint32_t periodSizeInMilliseconds{0};  ///< Mixing block length: lower is less latency, more CPU; 0: default.

    /// @brief Reads the keys `sampleRate`, `channels` and `periodSizeInMilliseconds`; the missing ones keep their
    /// default.
    /// @param settings The `miniaudio` section.
    /// @throws rtype::core::config::exceptions::SettingsException If a key is unknown, has the wrong type, or is not a
    /// non-negative integer.
    [[nodiscard]] static MiniaudioConfig fromSettings(const rtype::core::config::Settings& settings);
};
}  // namespace rtype::audio::miniaudio
