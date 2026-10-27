/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioResources
*/

#pragma once

#include <memory>
#include <optional>
#include <unordered_map>

#include "core/config/Settings.hpp"
#include "plugins/asset/AssetId.hpp"
#include "plugins/audio/AudioEvents.hpp"
#include "plugins/audio/AudioRegistry.hpp"
#include "plugins/audio/AudioTypes.hpp"
#include "plugins/audio/AudioVolumes.hpp"
#include "plugins/audio/IAudioDevice.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::audio {
/// @brief The voice an AudioSource plays, kept by the audio plugin.
struct SourceVoice {
    VoiceId voice;         ///< Stale once a one-shot source ended.
    asset::AssetId sound;  ///< The sound it was started with: another one restarts the voice.
};

/// @brief The started audio device and what the audio plugin plays on it, as the main-thread resource "rtype.Audio":
/// the backends' libraries must be called from one thread.
/// @details Null until the Startup system created the device, and on a server, which never does. Its moves are declared
/// noexcept: MSVC's std::unordered_map may throw when moved (it allocates a sentinel node), and a resource must not.
struct Audio {
    Audio() = default;
    ~Audio() = default;
    Audio(const Audio& other) = delete;
    Audio& operator=(const Audio& other) = delete;
    Audio(Audio&& other) noexcept = default;
    Audio& operator=(Audio&& other) noexcept = default;

    std::unique_ptr<IAudioDevice> device;                  ///< The backend the config names.
    float panDistance{400.0F};                             ///< From AudioConfig.
    std::unordered_map<ecs::Entity, SourceVoice> sources;  ///< The voice of each AudioSource that started.
    VoiceId music;                                         ///< The current music track.
    std::optional<PlayMusic> pendingMusic;                 ///< A PlayMusic waiting for its sound to load.
    std::optional<AudioVolumes> appliedVolumes;            ///< The volumes the device has; none before the first.
};
RTYPE_ECS_RESOURCE(Audio, "rtype.Audio");

/// @brief What the Startup system needs to create the device: the registered backends, and the configuration naming
/// the one to start. Inserted by AudioPlugin::build(), which opens nothing itself.
struct AudioSetup {
    std::unique_ptr<AudioRegistry> registry;  ///< The backends by name.
    core::config::Settings settings;          ///< `audio`, and a section per backend.
};
RTYPE_ECS_RESOURCE(AudioSetup, "rtype.AudioSetup");
}  // namespace rtype::plugins::audio
