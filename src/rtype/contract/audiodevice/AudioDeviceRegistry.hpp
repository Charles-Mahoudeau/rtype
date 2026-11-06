/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioDeviceRegistry
*/

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "core/config/Settings.hpp"
#include "rtype/contract/audiodevice/Export.hpp"
#include "rtype/contract/audiodevice/IAudioDevice.hpp"

namespace rtype::contract::audiodevice {
/// @brief The name an AudioDeviceRegistry is offered under to the libraries.
inline constexpr std::string_view kAudioDeviceRegistryService = "rtype.contract.audiodevices";

/// @brief Audio devices by name, so the configuration picks one instead of the code.
///
/// @details Each module registers its device (rtype::audio::miniaudio::registerAudio()...). A factory receives the
/// section named after it, which it checks and turns into its own typed configuration, and the voices to play at once.
class RTYPE_CONTRACT_AUDIODEVICE_API AudioDeviceRegistry {
  public:
    /// @param section The device's own section (`miniaudio`...), to check and read.
    /// @param maxVoices Voices playing at once.
    /// @throws Anything if the device cannot open.
    using Factory =
        std::function<std::unique_ptr<IAudioDevice>(const core::config::Settings& section, std::uint32_t maxVoices)>;

    AudioDeviceRegistry() = default;
    ~AudioDeviceRegistry() = default;
    AudioDeviceRegistry(const AudioDeviceRegistry& other) = delete;
    AudioDeviceRegistry& operator=(const AudioDeviceRegistry& other) = delete;
    AudioDeviceRegistry(AudioDeviceRegistry&& other) noexcept = default;
    AudioDeviceRegistry& operator=(AudioDeviceRegistry&& other) noexcept = default;

    /// @return The registered names, sorted.
    [[nodiscard]] std::vector<std::string> getNames() const;

    /// @return The factory registered under @p name, or nullptr.
    [[nodiscard]] const Factory* find(std::string_view name) const;

    /// @brief Registers a device under a name, replacing any previous one with the same name.
    void add(std::string name, Factory factory);

  private:
    std::map<std::string, Factory, std::less<>> _factories;  ///< Factories by name.
};
}  // namespace rtype::contract::audiodevice
