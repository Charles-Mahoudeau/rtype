/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MiniaudioDevice
*/

#include "audio/miniaudio/MiniaudioDevice.hpp"

#include <miniaudio.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <span>
#include <utility>

#include "audio/miniaudio/MiniaudioConfig.hpp"
#include "audio/miniaudio/core/Sound.hpp"
#include "audio/miniaudio/core/SoundGroup.hpp"
#include "audio/miniaudio/exceptions/MiniaudioException.hpp"
#include "rtype/contract/audiodevice/AudioTypes.hpp"

namespace rtype::audio::miniaudio {

MiniaudioDevice::MiniaudioDevice(std::uint32_t maxVoices, const MiniaudioConfig& miniaudio)
    : _engine{miniaudio}, _buses{core::SoundGroup{_engine}, core::SoundGroup{_engine}}, _voices{maxVoices} {
    // TODO: set world panDistance
}

MiniaudioDevice::~MiniaudioDevice() {
    _voices.clear();
    for (auto& [id, sound] : _sounds) {
        releaseSound(sound);
    }
}

contract::audiodevice::SoundId MiniaudioDevice::createSound(std::span<const std::byte> encoded,
                                                            const contract::audiodevice::SoundDesc& desc) {
    return addSound(LoadedSound{
        .name = std::format("rtype:sound:{}", _nextSound), .desc = desc, .encoded = {encoded.begin(), encoded.end()}});
}

contract::audiodevice::SoundId MiniaudioDevice::createSound(const std::filesystem::path& path,
                                                            const contract::audiodevice::SoundDesc& desc) {
    return addSound(LoadedSound{.name = path.string(), .desc = desc});
}

void MiniaudioDevice::destroySound(contract::audiodevice::SoundId sound) {
    const auto found = _sounds.find(sound.value);
    if (found == _sounds.end()) {
        return;
    }
    _voices.stopSound(sound);
    releaseSound(found->second);
    _sounds.erase(found);
}

contract::audiodevice::VoiceId MiniaudioDevice::play(contract::audiodevice::SoundId sound,
                                                     const contract::audiodevice::PlayParams& params) {
    const auto found = _sounds.find(sound.value);
    if (found == _sounds.end()) {
        return {};
    }
    const LoadedSound& loaded = found->second;
    std::unique_ptr<core::Sound> voice;
    try {
        voice = loaded.desc.stream
                    ? std::make_unique<core::Sound>(_engine, loaded.name, flagsOf(loaded.desc), &groupOf(params.bus))
                    : std::make_unique<core::Sound>(_engine, *loaded.model, flagsOf(loaded.desc), &groupOf(params.bus));
    } catch (const exceptions::MiniaudioException& error) {
        std::cerr << "[MiniaudioDevice] " << error.what() << '\n';
        return {};
    }
    ma_sound* raw = voice->get();
    ma_sound_set_volume(raw, params.volume);
    ma_sound_set_pitch(raw, params.pitch);
    ma_sound_set_pan(raw, params.pan);
    ma_sound_set_looping(raw, params.loop ? MA_TRUE : MA_FALSE);
    if (params.fadeIn > 0.0F) {
        ma_sound_set_fade_in_milliseconds(raw, 0.0F, 1.0F,
                                          static_cast<ma_uint64>(static_cast<double>(params.fadeIn) * 1000.0));
    }
    return _voices.add(std::move(voice), sound, params.bus);
}

void MiniaudioDevice::stop(contract::audiodevice::VoiceId voice, float fadeOut) { _voices.stop(voice, fadeOut); }

void MiniaudioDevice::setPaused(contract::audiodevice::VoiceId voice, bool paused) {
    if (core::Sound* found = _voices.find(voice)) {
        static_cast<void>(paused ? ma_sound_stop(found->get()) : ma_sound_start(found->get()));
    }
}

void MiniaudioDevice::setVolume(contract::audiodevice::VoiceId voice, float volume) {
    if (core::Sound* found = _voices.find(voice)) {
        ma_sound_set_volume(found->get(), volume);
    }
}

void MiniaudioDevice::setPitch(contract::audiodevice::VoiceId voice, float pitch) {
    if (core::Sound* found = _voices.find(voice)) {
        ma_sound_set_pitch(found->get(), pitch);
    }
}

void MiniaudioDevice::setPan(contract::audiodevice::VoiceId voice, float pan) {
    if (core::Sound* found = _voices.find(voice)) {
        ma_sound_set_pan(found->get(), pan);
    }
}

bool MiniaudioDevice::isPlaying(contract::audiodevice::VoiceId voice) const { return _voices.isPlaying(voice); }

void MiniaudioDevice::setMasterVolume(float volume) { _engine.setVolume(volume); }

void MiniaudioDevice::setBusVolume(contract::audiodevice::Bus bus, float volume) { groupOf(bus).setVolume(volume); }

void MiniaudioDevice::update() { _voices.update(); }

core::SoundGroup& MiniaudioDevice::groupOf(contract::audiodevice::Bus bus) {
    if (static_cast<std::size_t>(bus) >= _buses.size()) {
        throw std::out_of_range{"Bus index out of range"};
    }
    return _buses.at(static_cast<std::size_t>(bus));
}

contract::audiodevice::SoundId MiniaudioDevice::addSound(LoadedSound sound) {
    ma_resource_manager* resources = ma_engine_get_resource_manager(_engine.get());
    if (!sound.encoded.empty()) {
        const ma_result result = ma_resource_manager_register_encoded_data(resources, sound.name.c_str(),
                                                                           sound.encoded.data(), sound.encoded.size());
        if (result != MA_SUCCESS) {
            throw exceptions::MiniaudioException("Failed to register a sound given in memory", result);
        }
    }
    try {
        sound.model = std::make_unique<core::Sound>(_engine, sound.name, flagsOf(sound.desc), nullptr);
    } catch (...) {
        releaseSound(sound);
        throw;
    }
    const contract::audiodevice::SoundId id{.value = _nextSound++};
    _sounds.emplace(id.value, std::move(sound));
    return id;
}

void MiniaudioDevice::releaseSound(LoadedSound& sound) noexcept {
    sound.model.reset();
    if (!sound.encoded.empty()) {
        static_cast<void>(
            ma_resource_manager_unregister_data(ma_engine_get_resource_manager(_engine.get()), sound.name.c_str()));
        sound.encoded.clear();
    }
}

ma_uint32 MiniaudioDevice::flagsOf(const contract::audiodevice::SoundDesc& desc) noexcept {
    return (desc.stream ? MA_SOUND_FLAG_STREAM : MA_SOUND_FLAG_DECODE) | MA_SOUND_FLAG_NO_SPATIALIZATION;
}
}  // namespace rtype::audio::miniaudio
