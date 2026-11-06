/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include "core/library/LibraryEntry.hpp"
#include "rtype/audio/miniaudio/Registration.hpp"
#include "rtype/contract/audiodevice/AudioDeviceRegistry.hpp"

RTYPE_LIBRARY(registrar) {
    if (auto* registry = registrar.find<rtype::contract::audiodevice::AudioDeviceRegistry>(
            rtype::contract::audiodevice::kAudioDeviceRegistryService)) {
        rtype::audio::miniaudio::registerAudio(*registry);
    }
}
