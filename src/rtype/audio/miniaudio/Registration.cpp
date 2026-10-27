/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Registration
*/

#include "audio/miniaudio/Registration.hpp"

#include <memory>

#include "audio/miniaudio/MiniaudioConfig.hpp"
#include "audio/miniaudio/MiniaudioDevice.hpp"
#include "plugins/audio/AudioConfig.hpp"
#include "plugins/audio/AudioRegistry.hpp"
#include "plugins/audio/IAudioDevice.hpp"
#include "plugins/config/Settings.hpp"

namespace rtype::audio::miniaudio {

void registerAudio(plugins::audio::AudioRegistry& registry) {
    registry.add("miniaudio",
                 [](const plugins::config::Settings& section,
                    const plugins::audio::AudioConfig& config) -> std::unique_ptr<plugins::audio::IAudioDevice> {
                     return std::make_unique<MiniaudioDevice>(config, MiniaudioConfig::fromSettings(section));
                 });
}

}  // namespace rtype::audio::miniaudio
