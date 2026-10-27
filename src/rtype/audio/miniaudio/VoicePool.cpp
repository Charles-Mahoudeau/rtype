/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** VoicePool
*/

#include "audio/miniaudio/VoicePool.hpp"

#include <miniaudio.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "audio/miniaudio/core/Sound.hpp"
#include "plugins/audio/AudioTypes.hpp"

namespace rtype::audio::miniaudio {

VoicePool::VoicePool(std::uint32_t maxVoices) : _maxVoices{maxVoices} {}

plugins::audio::VoiceId VoicePool::add(std::unique_ptr<core::Sound> voice, plugins::audio::SoundId sound,
                                       plugins::audio::Bus bus) {
    const std::optional<std::size_t> index = takeSlot();
    if (!index || ma_sound_start(voice->get()) != MA_SUCCESS) {
        return {};
    }
    Slot& slot = _slots.at(*index);
    slot = Slot{.generation = slot.generation + 1,
                .sound = sound,
                .bus = bus,
                .startedAt = _nextStart++,
                .voice = std::move(voice)};
    return {.index = static_cast<std::uint32_t>(*index), .generation = slot.generation};
}

core::Sound* VoicePool::find(plugins::audio::VoiceId voice) noexcept {
    const Slot* slot = findSlot(voice);
    return slot != nullptr && !slot->stopping ? slot->voice.get() : nullptr;
}

bool VoicePool::isPlaying(plugins::audio::VoiceId voice) const noexcept {
    const Slot* slot = findSlot(voice);
    return slot != nullptr && !slot->stopping && ma_sound_at_end(slot->voice->get()) == MA_FALSE;
}

void VoicePool::stop(plugins::audio::VoiceId voice, float fadeOut) {
    Slot* slot = findSlot(voice);
    if (slot == nullptr || slot->stopping) {
        return;
    }
    if (fadeOut > 0.0F) {
        static_cast<void>(ma_sound_stop_with_fade_in_milliseconds(
            slot->voice->get(), static_cast<ma_uint64>(static_cast<double>(fadeOut) * 1000.0)));
        slot->stopping = true;
    } else {
        slot->voice.reset();
    }
}

void VoicePool::stopSound(plugins::audio::SoundId sound) noexcept {
    for (Slot& slot : _slots) {
        if (slot.voice && slot.sound == sound) {
            slot.voice.reset();
        }
    }
}

void VoicePool::clear() noexcept {
    for (Slot& slot : _slots) {
        slot.voice.reset();
    }
}

void VoicePool::update() noexcept {
    for (Slot& slot : _slots) {
        if (!slot.voice) {
            continue;
        }
        const bool ended = ma_sound_at_end(slot.voice->get()) == MA_TRUE;
        const bool fadedOut = slot.stopping && ma_sound_is_playing(slot.voice->get()) == MA_FALSE;
        if (ended || fadedOut) {
            slot.voice.reset();
        }
    }
}

VoicePool::Slot* VoicePool::findSlot(plugins::audio::VoiceId voice) noexcept {
    if (voice.index >= _slots.size()) {
        return nullptr;
    }
    Slot& slot = _slots.at(voice.index);
    return slot.voice && slot.generation == voice.generation ? &slot : nullptr;
}

const VoicePool::Slot* VoicePool::findSlot(plugins::audio::VoiceId voice) const noexcept {
    if (voice.index >= _slots.size()) {
        return nullptr;
    }
    const Slot& slot = _slots.at(voice.index);
    return slot.voice && slot.generation == voice.generation ? &slot : nullptr;
}

std::optional<std::size_t> VoicePool::takeSlot() {
    std::optional<std::size_t> oldestEffect;
    for (std::size_t index = 0; index < _slots.size(); ++index) {
        const Slot& slot = _slots.at(index);
        if (!slot.voice) {
            return index;
        }
        if (slot.bus != plugins::audio::Bus::kMusic &&
            (!oldestEffect || slot.startedAt < _slots.at(*oldestEffect).startedAt)) {
            oldestEffect = index;
        }
    }
    if (_slots.size() < _maxVoices) {
        _slots.emplace_back();
        return _slots.size() - 1;
    }
    if (oldestEffect) {
        _slots.at(*oldestEffect).voice.reset();
    }
    return oldestEffect;
}
}  // namespace rtype::audio::miniaudio
