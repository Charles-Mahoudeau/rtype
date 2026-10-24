/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** VoicePool
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "audio/miniaudio/core/Sound.hpp"
#include "plugins/audio/AudioTypes.hpp"

namespace rtype::audio::miniaudio {
/// @brief The voices playing: a table of slots, each holding one core::Sound, handed out as generational VoiceIds.
/// @details A VoiceId is a slot index and the slot's generation, bumped each time the slot is reused, so an id kept
/// after its voice ended never controls the next one: every call on a stale id does nothing. Ended voices are reclaimed
/// by update(), on the main thread: no callback comes from miniaudio's audio thread. Beyond maxVoices, the oldest
/// effect is stolen; music never is.
class VoicePool {
  public:
    /// @param maxVoices Voices playing at once, from AudioConfig.
    explicit VoicePool(std::uint32_t maxVoices);
    ~VoicePool() = default;
    VoicePool(const VoicePool& other) = delete;
    VoicePool& operator=(const VoicePool& other) = delete;
    VoicePool(VoicePool&& other) = delete;
    VoicePool& operator=(VoicePool&& other) = delete;

    /// @brief Starts @p voice in a free slot, or in the slot of the oldest effect if every slot is taken.
    /// @param voice A sound ready to play: volume, pitch, pan, looping and fade already set, not started yet.
    /// @param sound The sound it plays, for stopSound().
    /// @param bus Its bus: a music voice is never stolen.
    /// @return Its id, or a stale id if every slot holds music, or if it cannot start.
    plugins::audio::VoiceId add(std::unique_ptr<core::Sound> voice, plugins::audio::SoundId sound,
                                plugins::audio::Bus bus);

    /// @return The sound of the voice @p voice names, or nullptr if it is stale (ended, stopped, never handed out).
    [[nodiscard]] core::Sound* find(plugins::audio::VoiceId voice) noexcept;

    /// @return False once the voice has ended or been stopped, even while it fades out.
    [[nodiscard]] bool isPlaying(plugins::audio::VoiceId voice) const noexcept;

    /// @brief Stops the voice: at once, or fading out over @p fadeOut seconds, its slot being reclaimed by update().
    void stop(plugins::audio::VoiceId voice, float fadeOut);

    /// @brief Stops and frees every voice playing @p sound, before the sound itself is destroyed.
    void stopSound(plugins::audio::SoundId sound) noexcept;

    /// @brief Frees every voice.
    void clear() noexcept;

    /// @brief Reclaims the slots of the voices that ended, and of those whose fade out is over.
    void update() noexcept;

  private:
    /// @brief One slot of the table.
    struct Slot {
        std::uint32_t generation{0};         ///< Of the voice in the slot; 0 is never handed out.
        plugins::audio::SoundId sound;       ///< The sound it plays.
        plugins::audio::Bus bus{};           ///< Music is never stolen.
        std::uint64_t startedAt{0};          ///< Start order, to steal the oldest effect.
        bool stopping{false};                ///< Fading out after stop(): no longer playing for the plugin.
        std::unique_ptr<core::Sound> voice;  ///< The voice; null when the slot is free.
    };

    /// @return The slot @p voice names, or nullptr if it is stale.
    [[nodiscard]] Slot* findSlot(plugins::audio::VoiceId voice) noexcept;

    /// @return The slot @p voice names, or nullptr if it is stale.
    [[nodiscard]] const Slot* findSlot(plugins::audio::VoiceId voice) const noexcept;

    /// @return The index of a free slot, a new one, or the oldest effect's (freed); nothing if every slot is music.
    [[nodiscard]] std::optional<std::size_t> takeSlot();

    std::uint32_t _maxVoices;     ///< Size limit of _slots.
    std::uint64_t _nextStart{0};  ///< The start order of the next voice.
    std::vector<Slot> _slots;     ///< The table, indexed by VoiceId::index.
};
}  // namespace rtype::audio::miniaudio
