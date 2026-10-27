/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Sound
*/

#include "audio/miniaudio/core/Sound.hpp"

#include <miniaudio.h>

#include <format>
#include <string>

#include "audio/miniaudio/core/Engine.hpp"
#include "audio/miniaudio/core/SoundGroup.hpp"
#include "audio/miniaudio/exceptions/MiniaudioException.hpp"

namespace rtype::audio::miniaudio::core {

Sound::Sound(Engine& engine, const std::string& name, ma_uint32 flags, SoundGroup* group) {
    const ma_result result = ma_sound_init_from_file(engine.get(), name.c_str(), flags,
                                                     group != nullptr ? group->get() : nullptr, nullptr, &_sound);
    if (result != MA_SUCCESS) {
        throw exceptions::MiniaudioException(std::format("Failed to load the sound '{}'", name), result);
    }
}

Sound::Sound(Engine& engine, const Sound& model, ma_uint32 flags, SoundGroup* group) {
    const ma_result result =
        ma_sound_init_copy(engine.get(), model.get(), flags, group != nullptr ? group->get() : nullptr, &_sound);
    if (result != MA_SUCCESS) {
        throw exceptions::MiniaudioException("Failed to copy a sound", result);
    }
}

Sound::~Sound() { ma_sound_uninit(&_sound); }

ma_sound* Sound::get() noexcept { return &_sound; }

const ma_sound* Sound::get() const noexcept { return &_sound; }

}  // namespace rtype::audio::miniaudio::core
