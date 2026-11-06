/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Registration
*/

#include "audio/miniaudio/Registration.hpp"

#include <cstdint>
#include <memory>

#include "audio/miniaudio/MiniaudioConfig.hpp"
#include "audio/miniaudio/MiniaudioDevice.hpp"
#include "core/config/Settings.hpp"
#include "rtype/contract/audiodevice/AudioDeviceRegistry.hpp"
#include "rtype/contract/audiodevice/IAudioDevice.hpp"

namespace rtype::audio::miniaudio {

void registerAudio(contract::audiodevice::AudioDeviceRegistry& registry) {
    registry.add("miniaudio",
                 [](const rtype::core::config::Settings& section,
                    std::uint32_t maxVoices) -> std::unique_ptr<contract::audiodevice::IAudioDevice> {
                     return std::make_unique<MiniaudioDevice>(maxVoices, MiniaudioConfig::fromSettings(section));
                 });
}

}  // namespace rtype::audio::miniaudio
