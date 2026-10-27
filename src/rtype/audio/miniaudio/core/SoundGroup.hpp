/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SoundGroup
*/

#pragma once

#include <miniaudio.h>

#include "audio/miniaudio/core/Engine.hpp"

namespace rtype::audio::miniaudio::core {
/// @brief Owns a ma_sound_group: a bus that mixes the sounds attached to it, with its own volume, attached to an
/// engine's output.
class SoundGroup {
  public:
    /// @param engine The engine whose output the group plays into; must outlive the group.
    /// @throws exceptions::MiniaudioException If the group cannot be created.
    explicit SoundGroup(Engine& engine);
    ~SoundGroup();
    SoundGroup(const SoundGroup& other) = delete;
    SoundGroup& operator=(const SoundGroup& other) = delete;
    SoundGroup(SoundGroup&& other) = delete;
    SoundGroup& operator=(SoundGroup&& other) = delete;

    /// @return The group, for the sounds attached to it.
    [[nodiscard]] ma_sound_group* get() noexcept;

    /// @brief Sets the master volume of the group, affecting all sounds attached to it.
    /// @param volume The volume, 0.0 (silence) to 1.0 (full volume); values above 1.0 are allowed, but may distort.
    void setVolume(float volume);

  private:
    ma_sound_group _group{};  ///< Never moves: the sounds attached to it point to it.
};
}  // namespace rtype::audio::miniaudio::core
