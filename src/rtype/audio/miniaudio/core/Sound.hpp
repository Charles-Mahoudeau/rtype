/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Sound
*/

#pragma once

#include <miniaudio.h>

#include <string>

#include "audio/miniaudio/core/Engine.hpp"
#include "audio/miniaudio/core/SoundGroup.hpp"

namespace rtype::audio::miniaudio::core {
/// @brief Owns a ma_sound: a sound loaded through the engine's resource manager, or a copy of one sharing its data.
/// @details Used both as the model createSound() keeps (attached to no group, never played) and as a voice play()
/// starts in a group.
class Sound {
  public:
    /// @brief Loads the file, or the encoded data registered, under @p name.
    /// @param engine The engine whose resource manager loads it; must outlive the sound.
    /// @param name A file path, or a name given to ma_resource_manager_register_encoded_data().
    /// @param flags MA_SOUND_FLAG_* bits: MA_SOUND_FLAG_DECODE or MA_SOUND_FLAG_STREAM, and so on.
    /// @param group The group it plays in, or nullptr for the engine's output; must outlive the sound.
    /// @throws exceptions::MiniaudioException If it cannot be opened or decoded.
    Sound(Engine& engine, const std::string& name, ma_uint32 flags, SoundGroup* group);

    /// @brief A new sound sharing the decoded data of @p model, without loading it again.
    /// @param model A sound loaded with MA_SOUND_FLAG_DECODE: a streamed sound cannot be copied.
    /// @throws exceptions::MiniaudioException If @p model cannot be copied.
    Sound(Engine& engine, const Sound& model, ma_uint32 flags, SoundGroup* group);

    ~Sound();
    Sound(const Sound& other) = delete;
    Sound& operator=(const Sound& other) = delete;
    Sound(Sound&& other) = delete;
    Sound& operator=(Sound&& other) = delete;

    /// @return The sound, for the ma_sound_* calls.
    [[nodiscard]] ma_sound* get() noexcept;

    /// @return The sound, for the ma_sound_* calls.
    [[nodiscard]] const ma_sound* get() const noexcept;

  private:
    ma_sound _sound{};  ///< Never moves: its group and the resource manager point to it.
};
}  // namespace rtype::audio::miniaudio::core
