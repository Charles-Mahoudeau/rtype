/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MiniaudioDevice
*/

#include "audio/miniaudio/MiniaudioDevice.hpp"

#include <miniaudio.h>

#include <cstddef>
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
#include "plugins/audio/AudioConfig.hpp"
#include "plugins/audio/AudioTypes.hpp"

namespace rtype::audio::miniaudio {

MiniaudioDevice::MiniaudioDevice(const plugins::audio::AudioConfig& config, const MiniaudioConfig& miniaudio)
    : _engine{miniaudio}, _buses{core::SoundGroup{_engine}, core::SoundGroup{_engine}}, _voices{config.maxVoices} {
    const auto& volumes = config.volume;
    _engine.setVolume(volumes.master);
    groupOf(plugins::audio::Bus::kMusic).setVolume(volumes.music);
    groupOf(plugins::audio::Bus::kEffects).setVolume(volumes.effects);

    // TODO: set world panDistance
}

MiniaudioDevice::~MiniaudioDevice() {
    _voices.clear();
    for (auto& [id, sound] : _sounds) {
        releaseSound(sound);
    }
}

plugins::audio::SoundId MiniaudioDevice::createSound(std::span<const std::byte> encoded,
                                                     const plugins::audio::SoundDesc& desc) {
    return addSound(LoadedSound{
        .name = std::format("rtype:sound:{}", _nextSound), .desc = desc, .encoded = {encoded.begin(), encoded.end()}});
}

plugins::audio::SoundId MiniaudioDevice::createSound(const std::filesystem::path& path,
                                                     const plugins::audio::SoundDesc& desc) {
    return addSound(LoadedSound{.name = path.string(), .desc = desc});
}

void MiniaudioDevice::destroySound(plugins::audio::SoundId sound) {
    const auto found = _sounds.find(sound.value);
    if (found == _sounds.end()) {
        return;
    }
    _voices.stopSound(sound);
    releaseSound(found->second);
    _sounds.erase(found);
}

plugins::audio::VoiceId MiniaudioDevice::play(plugins::audio::SoundId sound, const plugins::audio::PlayParams& params) {
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

void MiniaudioDevice::stop(plugins::audio::VoiceId voice, float fadeOut) { _voices.stop(voice, fadeOut); }

void MiniaudioDevice::setPaused(plugins::audio::VoiceId voice, bool paused) {
    if (core::Sound* found = _voices.find(voice)) {
        static_cast<void>(paused ? ma_sound_stop(found->get()) : ma_sound_start(found->get()));
    }
}

void MiniaudioDevice::setVolume(plugins::audio::VoiceId voice, float volume) {
    if (core::Sound* found = _voices.find(voice)) {
        ma_sound_set_volume(found->get(), volume);
    }
}

void MiniaudioDevice::setPitch(plugins::audio::VoiceId voice, float pitch) {
    if (core::Sound* found = _voices.find(voice)) {
        ma_sound_set_pitch(found->get(), pitch);
    }
}

void MiniaudioDevice::setPan(plugins::audio::VoiceId voice, float pan) {
    if (core::Sound* found = _voices.find(voice)) {
        ma_sound_set_pan(found->get(), pan);
    }
}

bool MiniaudioDevice::isPlaying(plugins::audio::VoiceId voice) const { return _voices.isPlaying(voice); }

void MiniaudioDevice::setMasterVolume(float volume) { _engine.setVolume(volume); }

void MiniaudioDevice::setBusVolume(plugins::audio::Bus bus, float volume) { groupOf(bus).setVolume(volume); }

void MiniaudioDevice::update() { _voices.update(); }

core::SoundGroup& MiniaudioDevice::groupOf(plugins::audio::Bus bus) {
    if (static_cast<std::size_t>(bus) >= _buses.size()) {
        throw std::out_of_range{"Bus index out of range"};
    }
    return _buses.at(static_cast<std::size_t>(bus));
}

plugins::audio::SoundId MiniaudioDevice::addSound(LoadedSound sound) {
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
    const plugins::audio::SoundId id{.value = _nextSound++};
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

ma_uint32 MiniaudioDevice::flagsOf(const plugins::audio::SoundDesc& desc) noexcept {
    return (desc.stream ? MA_SOUND_FLAG_STREAM : MA_SOUND_FLAG_DECODE) | MA_SOUND_FLAG_NO_SPATIALIZATION;
}
}  // namespace rtype::audio::miniaudio
