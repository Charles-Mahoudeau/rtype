/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <string>
#include <utility>

#include "core/config/Settings.hpp"
#include "plugins/audio/AudioPlugin.hpp"
#include "plugins/audio/AudioRegistry.hpp"
#include "rtype/contract/audiodevice/AudioDeviceRegistry.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    using rtype::plugins::audio::AudioPlugin;
    using rtype::plugins::audio::AudioRegistry;
    // Filled by the backend libraries, then given to the plugin when it is built.
    auto audio = std::make_shared<std::unique_ptr<AudioRegistry>>(std::make_unique<AudioRegistry>());
    registrar.getLibrary().provide(rtype::contract::audiodevice::kAudioDeviceRegistryService, (*audio)->devices);
    registrar.getPlugins().add(std::string{AudioPlugin::kName}, [audio](const rtype::core::config::Settings& settings) {
        return std::make_unique<AudioPlugin>(std::move(*audio), settings);
    });
}
