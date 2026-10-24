/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioAssets
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "plugins/asset/AssetId.hpp"
#include "plugins/audio/AudioTypes.hpp"
#include "plugins/audio/IAudioDevice.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::audio {
/// @brief Where a sound is in its loading.
enum class SoundState : std::uint8_t {
    kQueued,  ///< Asked for, waiting for the audio device to create it.
    kLoaded,  ///< Created by the audio device: its SoundId can be played.
    kFailed,  ///< Could not be loaded; the entry says why.
};

/// @brief The sounds, as the ECS resource "rtype.AudioAssets": systems ask for a sound by path and get its AssetId at
/// once, components and events keep that id, and the audio plugin loads the queued sounds.
///
/// @details load() records the sound as queued and returns its id, without touching the audio device: gameplay systems
/// need nothing but this resource, and a server without a device gets the same ids. Each frame, the audio plugin calls
/// updateSounds(), which creates the queued sounds and destroys the unloaded ones. Unlike TextureAssets, nothing is
/// decoded here: the device decodes, and streams a sound loaded with SoundDesc::stream. Lookups are by id, in constant
/// time.
/// @code
/// void spawnShip(rtype::ecs::Commands& commands, rtype::ecs::ResMut<rtype::plugins::audio::AudioAssets> sounds)
/// {
///     const rtype::plugins::asset::AssetId engine = sounds->load("assets/sounds/engine.ogg");
///     commands.spawn(rtype::plugins::audio::AudioSource{.sound = engine, .loop = true});
/// }
/// @endcode
class AudioAssets {
  public:
    /// @brief One sound.
    struct Entry {
        std::string path;                       ///< Its path, or its name for a sound given in memory.
        SoundDesc desc;                         ///< How it is loaded: decoded once, or streamed.
        SoundState state{SoundState::kQueued};  ///< Where it is in its loading.
        SoundId sound;                          ///< The device's id, once loaded.
        std::vector<std::byte> encoded;         ///< The file's bytes, for a sound given in memory; freed once loaded.
        std::string error;                      ///< Why it failed to load.
    };

    AudioAssets() = default;
    ~AudioAssets() = default;
    AudioAssets(const AudioAssets& other) = delete;
    AudioAssets& operator=(const AudioAssets& other) = delete;
    AudioAssets(AudioAssets&& other) noexcept = default;
    AudioAssets& operator=(AudioAssets&& other) noexcept = default;

    /// @return The number of sounds known, whatever their state.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @brief Asks for the sound file at @p path (WAV, OGG, MP3, FLAC), opened by the device in updateSounds().
    /// @param desc How to load it. Ignored if the sound was already asked for: the first request wins.
    /// @return Its id (AssetId::fromPath()), at once: put it in components and events, the sound plays once loaded.
    /// @throws exceptions::AudioLoadException If another path already has this id (a hash collision).
    asset::AssetId load(const std::filesystem::path& path, const SoundDesc& desc = {});

    /// @brief Asks for a sound already in memory (embedded, downloaded), known by @p name.
    /// @param encoded The sound file's bytes (OGG...), copied.
    /// @return Its id (AssetId::fromPath(name)).
    /// @throws exceptions::AudioLoadException If another name already has this id (a hash collision).
    asset::AssetId load(const std::string& name, std::span<const std::byte> encoded, const SoundDesc& desc = {});

    /// @return The sound @p id names, or nullptr if it was never asked for or was unloaded.
    [[nodiscard]] const Entry* get(asset::AssetId id) const;

    /// @return The device's id of the sound @p id names, or nothing if it is not loaded (yet).
    [[nodiscard]] std::optional<SoundId> getSound(asset::AssetId id) const;

    /// @brief Forgets the sound @p id names; if it was loaded, the next updateSounds() destroys it, which stops its
    /// voices.
    /// @return False, doing nothing, if no sound has this id.
    bool unload(asset::AssetId id);

    /// @name For updateSounds()
    /// @{

    /// @return The ids of the queued sounds, in the order they were asked for.
    [[nodiscard]] const std::vector<asset::AssetId>& getQueued() const noexcept;

    /// @brief Records that the queued sound @p id was created as @p sound, and frees its in-memory bytes. Does nothing
    /// if @p id is not queued.
    void setLoaded(asset::AssetId id, SoundId sound);

    /// @brief Records that the queued sound @p id could not be loaded, and why. Does nothing if @p id is not queued.
    void setFailed(asset::AssetId id, std::string error);

    /// @return The sounds unloaded since the last call, for the device to destroy.
    [[nodiscard]] std::vector<SoundId> takeUnloaded();
    /// @}

  private:
    /// @brief Records a queued sound, or returns the id of the one already asked for.
    /// @throws exceptions::AudioLoadException If another path already has the id.
    asset::AssetId request(std::string path, std::vector<std::byte> encoded, const SoundDesc& desc);

    /// @return The queued entry @p id names, removed from the queue, or nullptr if it is not queued.
    Entry* dequeue(asset::AssetId id);

    std::unordered_map<asset::AssetId, Entry> _entries;  ///< Every sound asked for, by id.
    std::vector<asset::AssetId> _queued;                 ///< The queued sounds, in the order they were asked for.
    std::vector<SoundId> _unloaded;                      ///< Sounds to destroy, from unload().
};
RTYPE_ECS_RESOURCE(AudioAssets, "rtype.AudioAssets");

/// @brief Destroys the sounds unloaded since the last call, then loads the queued ones in @p device: from their file,
/// or from the bytes given to load(). Called by the audio plugin each frame, on the main thread.
/// @details A sound the device cannot open or decode is marked SoundState::kFailed with the reason, which is also
/// printed: one missing sound does not stop the game.
void updateSounds(AudioAssets& assets, IAudioDevice& device);
}  // namespace rtype::plugins::audio
