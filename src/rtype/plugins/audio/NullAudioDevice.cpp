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

#include "plugins/audio/AudioTypes.hpp"

namespace rtype::plugins::audio {

NullAudioDevice::NullAudioDevice(std::uint32_t maxVoices) : _maxVoices{maxVoices} {}

SoundId NullAudioDevice::createSound(std::span<const std::byte> /*encoded*/, const SoundDesc& /*desc*/) {
    const SoundId sound{.value = _nextSound++};
    _sounds.insert(sound.value);
    return sound;
}

SoundId NullAudioDevice::createSound(const std::filesystem::path& /*path*/, const SoundDesc& desc) {
    return createSound(std::span<const std::byte>{}, desc);
}

void NullAudioDevice::destroySound(SoundId sound) {
    if (_sounds.erase(sound.value) == 0) {
        return;
    }
    for (Voice& voice : _voices) {
        if (voice.sound == sound) {
            voice.playing = false;
        }
    }
}

VoiceId NullAudioDevice::play(SoundId sound, const PlayParams& params) {
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
    return VoiceId{.index = static_cast<std::uint32_t>(index), .generation = voice.generation};
}

void NullAudioDevice::stop(VoiceId voice, float /*fadeOut*/) {
    if (Voice* found = find(voice)) {
        found->playing = false;
    }
}

void NullAudioDevice::setPaused(VoiceId voice, bool paused) {
    if (Voice* found = find(voice)) {
        found->paused = paused;
    }
}

void NullAudioDevice::setVolume(VoiceId /*voice*/, float /*volume*/) {}

void NullAudioDevice::setPitch(VoiceId /*voice*/, float /*pitch*/) {}

void NullAudioDevice::setPan(VoiceId /*voice*/, float /*pan*/) {}

bool NullAudioDevice::isPlaying(VoiceId voice) const { return find(voice) != nullptr; }

void NullAudioDevice::setMasterVolume(float /*volume*/) {}

void NullAudioDevice::setBusVolume(Bus /*bus*/, float /*volume*/) {}

void NullAudioDevice::update() {
    for (Voice& voice : _voices) {
        if (voice.playing && !voice.loop && !voice.paused) {
            voice.playing = false;
        }
    }
}

NullAudioDevice::Voice* NullAudioDevice::find(VoiceId voice) {
    if (voice.index >= _voices.size()) {
        return nullptr;
    }
    Voice& found = _voices.at(voice.index);
    return found.playing && found.generation == voice.generation ? &found : nullptr;
}

const NullAudioDevice::Voice* NullAudioDevice::find(VoiceId voice) const {
    if (voice.index >= _voices.size()) {
        return nullptr;
    }
    const Voice& found = _voices.at(voice.index);
    return found.playing && found.generation == voice.generation ? &found : nullptr;
}
}  // namespace rtype::plugins::audio
