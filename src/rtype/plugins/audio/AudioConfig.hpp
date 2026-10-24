/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioConfig
*/

#pragma once

#include <cstdint>
#include <string>

#include "plugins/audio/AudioVolumes.hpp"
#include "plugins/config/Settings.hpp"

namespace rtype::plugins::audio {
/// @brief The settings every audio backend shares, read from the `audio` section of the config (fromSettings()). Each
/// backend also gets its own section, named after it (`miniaudio = { ... }`), which it reads itself.
/// @code{.lua}
/// audio = {
///     backend = "miniaudio",
///     required = false,
///     maxVoices = 64,
///     volume = { master = 1.0, music = 0.7, effects = 1.0 },
///     panDistance = 400,
/// },
/// @endcode
struct AudioConfig {
    std::string backend;          ///< The registered backend to create ("miniaudio", "null"...); empty if not chosen.
    bool required{false};         ///< If the device cannot open: false falls back to "null", true throws.
    std::uint32_t maxVoices{64};  ///< Voices playing at once; beyond that the backend steals one, or plays nothing.
    AudioVolumes volume;          ///< The starting master and bus volumes.
    float panDistance{400.0F};    ///< World units from the listener at which a sound is fully left or right.

    /// @brief Reads the keys `backend`, `required`, `maxVoices`, `volume.master`, `volume.music`, `volume.effects` and
    /// `panDistance`; the missing ones keep their default.
    /// @param settings The `audio` section.
    /// @throws config::exceptions::SettingsException If a key is unknown, has the wrong type or an invalid value.
    [[nodiscard]] static AudioConfig fromSettings(const config::Settings& settings);
};
}  // namespace rtype::plugins::audio
