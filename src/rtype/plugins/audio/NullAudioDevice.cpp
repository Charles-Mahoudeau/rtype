/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NullAudioDevice
*/

#include "plugins/audio/NullAudioDevice.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>

#include "rtype/contract/audiodevice/AudioTypes.hpp"

namespace rtype::plugins::audio {

NullAudioDevice::NullAudioDevice(std::uint32_t maxVoices) : _maxVoices{maxVoices} {}

contract::audiodevice::SoundId NullAudioDevice::createSound(std::span<const std::byte> /*encoded*/,
                                                            const contract::audiodevice::SoundDesc& /*desc*/) {
    const contract::audiodevice::SoundId sound{.value = _nextSound++};
    _sounds.insert(sound.value);
    return sound;
}

contract::audiodevice::SoundId NullAudioDevice::createSound(const std::filesystem::path& /*path*/,
                                                            const contract::audiodevice::SoundDesc& desc) {
    return createSound(std::span<const std::byte>{}, desc);
}

void NullAudioDevice::destroySound(contract::audiodevice::SoundId sound) {
    if (_sounds.erase(sound.value) == 0) {
        return;
    }
    for (Voice& voice : _voices) {
        if (voice.sound == sound) {
            voice.playing = false;
        }
    }
}

contract::audiodevice::VoiceId NullAudioDevice::play(contract::audiodevice::SoundId sound,
                                                     const contract::audiodevice::PlayParams& params) {
    if (!_sounds.contains(sound.value)) {
        return {};
    }
    std::size_t index = 0;
    while (index < _voices.size() && _voices.at(index).playing) {
        ++index;
    }
    if (index == _voices.size()) {
        if (_voices.size() >= _maxVoices) {
            return {};
        }
        _voices.emplace_back();
    }
    Voice& voice = _voices.at(index);
    voice = Voice{.generation = voice.generation + 1, .sound = sound, .playing = true, .loop = params.loop};
    return contract::audiodevice::VoiceId{.index = static_cast<std::uint32_t>(index), .generation = voice.generation};
}

void NullAudioDevice::stop(contract::audiodevice::VoiceId voice, float /*fadeOut*/) {
    if (Voice* found = find(voice)) {
        found->playing = false;
    }
}

void NullAudioDevice::setPaused(contract::audiodevice::VoiceId voice, bool paused) {
    if (Voice* found = find(voice)) {
        found->paused = paused;
    }
}

void NullAudioDevice::setVolume(contract::audiodevice::VoiceId /*voice*/, float /*volume*/) {}

void NullAudioDevice::setPitch(contract::audiodevice::VoiceId /*voice*/, float /*pitch*/) {}

void NullAudioDevice::setPan(contract::audiodevice::VoiceId /*voice*/, float /*pan*/) {}

bool NullAudioDevice::isPlaying(contract::audiodevice::VoiceId voice) const { return find(voice) != nullptr; }

void NullAudioDevice::setMasterVolume(float /*volume*/) {}

void NullAudioDevice::setBusVolume(contract::audiodevice::Bus /*bus*/, float /*volume*/) {}

void NullAudioDevice::update() {
    for (Voice& voice : _voices) {
        if (voice.playing && !voice.loop && !voice.paused) {
            voice.playing = false;
        }
    }
}

NullAudioDevice::Voice* NullAudioDevice::find(contract::audiodevice::VoiceId voice) {
    if (voice.index >= _voices.size()) {
        return nullptr;
    }
    Voice& found = _voices.at(voice.index);
    return found.playing && found.generation == voice.generation ? &found : nullptr;
}

const NullAudioDevice::Voice* NullAudioDevice::find(contract::audiodevice::VoiceId voice) const {
    if (voice.index >= _voices.size()) {
        return nullptr;
    }
    const Voice& found = _voices.at(voice.index);
    return found.playing && found.generation == voice.generation ? &found : nullptr;
}
}  // namespace rtype::plugins::audio
