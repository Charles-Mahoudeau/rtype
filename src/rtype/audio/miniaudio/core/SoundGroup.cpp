/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SoundGroup
*/

#include "audio/miniaudio/core/SoundGroup.hpp"

#include <miniaudio.h>

#include "audio/miniaudio/core/Engine.hpp"
#include "audio/miniaudio/exceptions/MiniaudioException.hpp"

namespace rtype::audio::miniaudio::core {

SoundGroup::SoundGroup(Engine& engine) {
    const ma_result result = ma_sound_group_init(engine.get(), 0, nullptr, &_group);
    if (result != MA_SUCCESS) {
        throw exceptions::MiniaudioException("Failed to initialize a sound group", result);
    }
}

SoundGroup::~SoundGroup() { ma_sound_group_uninit(&_group); }

ma_sound_group* SoundGroup::get() noexcept { return &_group; }

void SoundGroup::setVolume(float volume) { ma_sound_group_set_volume(&_group, volume); }

}  // namespace rtype::audio::miniaudio::core
