/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioPlugin
*/

#include "plugins/audio/AudioPlugin.hpp"

#include <algorithm>
#include <glm/ext/vector_float2.hpp>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "plugins/asset/AssetId.hpp"
#include "plugins/audio/AudioAssets.hpp"
#include "plugins/audio/AudioConfig.hpp"
#include "plugins/audio/AudioEvents.hpp"
#include "plugins/audio/AudioListener.hpp"
#include "plugins/audio/AudioRegistry.hpp"
#include "plugins/audio/AudioResources.hpp"
#include "plugins/audio/AudioSource.hpp"
#include "plugins/audio/AudioTypes.hpp"
#include "plugins/audio/AudioVolumes.hpp"
#include "plugins/audio/IAudioDevice.hpp"
#include "plugins/config/Settings.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/Schedule.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"

namespace rtype::plugins::audio {
namespace {
/// @brief The entities that hear: the first one placed is the listener.
using Listeners = ecs::Query<ecs::Optional<const transform::GlobalTransform>, ecs::Optional<const transform::Transform>,
                             ecs::With<AudioListener>>;

/// @return Where an entity is: its GlobalTransform, or its Transform while it has no GlobalTransform yet (spawned this
/// frame: the TransformPlugin's commands give it one at the end of PostUpdate); nothing if it has neither.
[[nodiscard]] std::optional<glm::vec2> positionOf(const transform::GlobalTransform* global,
                                                  const transform::Transform* local) {
    if (global != nullptr) {
        return global->position;
    }
    if (local != nullptr) {
        return local->position;
    }
    return std::nullopt;
}

/// @return Where the listener is, or nothing if no entity with an AudioListener is placed.
[[nodiscard]] std::optional<glm::vec2> findListener(Listeners& listeners) {
    for (auto [entity, global, local] : listeners) {
        if (const std::optional<glm::vec2> position = positionOf(global, local)) {
            return position;
        }
    }
    return std::nullopt;
}

/// @return The pan of a sound at @p position for a listener at @p listener: -1 left, 0 centre, 1 right; 0 when either
/// is unknown.
[[nodiscard]] float panOf(std::optional<glm::vec2> position, std::optional<glm::vec2> listener, float panDistance) {
    if (!position || !listener) {
        return 0.0F;
    }
    return std::clamp((position->x - listener->x) / panDistance, -1.0F, 1.0F);
}

/// @brief Gives the device the volumes that differ from those it has (all of them the first time).
void applyVolumes(Audio& audio, const AudioVolumes& volumes) {
    const std::optional<AudioVolumes> applied = audio.appliedVolumes;
    if (!applied || applied->master != volumes.master) {
        audio.device->setMasterVolume(volumes.master);
    }
    if (!applied || applied->music != volumes.music) {
        audio.device->setBusVolume(Bus::kMusic, volumes.music);
    }
    if (!applied || applied->effects != volumes.effects) {
        audio.device->setBusVolume(Bus::kEffects, volumes.effects);
    }
    audio.appliedVolumes = volumes;
}

/// @brief Stops the current music, if one plays, fading out over @p fadeOut seconds.
void stopMusic(Audio& audio, float fadeOut) {
    if (audio.music != VoiceId{}) {
        audio.device->stop(audio.music, fadeOut);
        audio.music = {};
    }
}

/// @brief Startup: creates the device the configuration names, and starts the volumes from the config.
void createDevice(ecs::NonSendMut<AudioSetup> setup, ecs::NonSendMut<Audio> audio, ecs::ResMut<AudioVolumes> volumes) {
    const AudioConfig config = AudioConfig::fromSettings(setup->settings.section("audio"));
    audio->device = setup->registry->createDevice(setup->settings);
    audio->panDistance = config.panDistance;
    *volumes = config.volume;
    applyVolumes(*audio, *volumes);
}

/// @brief PostUpdate: destroys the unloaded sounds and loads the queued ones.
void loadSounds(ecs::NonSendMut<Audio> audio, ecs::ResMut<AudioAssets> sounds) {
    if (audio->device) {
        updateSounds(*sounds, *audio->device);
    }
}

/// @brief Starts the pending music if its sound is loaded, crossfading from the current track; forgets it if its sound
/// failed or was never asked for.
void startPendingMusic(Audio& audio, const AudioAssets& sounds) {
    if (!audio.pendingMusic) {
        return;
    }
    const PlayMusic music = *audio.pendingMusic;
    if (const std::optional<SoundId> sound = sounds.getSound(music.sound)) {
        stopMusic(audio, music.crossfade);
        audio.music =
            audio.device->play(*sound, PlayParams{.bus = Bus::kMusic, .loop = true, .fadeIn = music.crossfade});
        audio.pendingMusic.reset();
        return;
    }
    const AudioAssets::Entry* entry = sounds.get(music.sound);
    if (entry == nullptr || entry->state == SoundState::kFailed) {
        std::cerr << "[AudioPlugin] The music " << (entry != nullptr ? entry->path : "asked for")
                  << " cannot play: its sound is not loaded\n";
        audio.pendingMusic.reset();
    }
}

/// @brief PostUpdate: plays the one-shot sounds, then the music events.
void playEvents(ecs::NonSendMut<Audio> audio, ecs::Res<AudioAssets> sounds, ecs::EventReader<PlaySound> playSounds,
                ecs::EventReader<StopMusic> stopMusics, ecs::EventReader<PlayMusic> playMusics, Listeners listeners) {
    if (!audio->device) {
        return;
    }
    const std::optional<glm::vec2> listener = findListener(listeners);
    for (const PlaySound& event : playSounds) {
        if (const std::optional<SoundId> sound = sounds->getSound(event.sound)) {
            static_cast<void>(
                audio->device->play(*sound, PlayParams{.bus = event.bus,
                                                       .volume = event.volume,
                                                       .pitch = event.pitch,
                                                       .pan = panOf(event.position, listener, audio->panDistance)}));
        }
    }
    for (const StopMusic& event : stopMusics) {
        stopMusic(*audio, event.fadeOut);
        audio->pendingMusic.reset();
    }
    for (const PlayMusic& event : playMusics) {
        audio->pendingMusic = event;
    }
    startPendingMusic(*audio, *sounds);
}

/// @brief PostUpdate: keeps one voice per AudioSource, and stops the voices of the sources that went.
void followSources(ecs::NonSendMut<Audio> audio, ecs::Res<AudioAssets> sounds,
                   ecs::Query<const AudioSource, ecs::Optional<const transform::GlobalTransform>,
                              ecs::Optional<const transform::Transform>>
                       sources,
                   Listeners listeners) {
    if (!audio->device) {
        return;
    }
    IAudioDevice& device = *audio->device;
    const std::optional<glm::vec2> listener = findListener(listeners);
    std::unordered_map<ecs::Entity, SourceVoice> kept;
    for (auto [entity, source, global, local] : sources) {
        const std::optional<glm::vec2> position = source.spatial ? positionOf(global, local) : std::nullopt;
        const float pan = panOf(position, listener, audio->panDistance);
        const auto known = audio->sources.find(entity);
        if (known != audio->sources.end() && known->second.sound == source.sound) {
            device.setVolume(known->second.voice, source.volume);
            device.setPitch(known->second.voice, source.pitch);
            device.setPan(known->second.voice, pan);
            kept.emplace(entity, known->second);
            continue;
        }
        if (known != audio->sources.end()) {
            device.stop(known->second.voice);
        }
        const std::optional<SoundId> sound = sounds->getSound(source.sound);
        if (!sound) {
            continue;  // Not loaded yet: tried again next frame.
        }
        const VoiceId voice = device.play(
            *sound,
            PlayParams{
                .bus = source.bus, .volume = source.volume, .pitch = source.pitch, .pan = pan, .loop = source.loop});
        kept.emplace(entity, SourceVoice{.voice = voice, .sound = source.sound});
    }
    for (const auto& [entity, gone] : audio->sources) {
        if (!kept.contains(entity)) {
            device.stop(gone.voice);
        }
    }
    audio->sources = std::move(kept);
}

/// @brief PostUpdate: applies the volumes that changed, then lets the backend reclaim its voices.
void updateDevice(ecs::NonSendMut<Audio> audio, ecs::Res<AudioVolumes> volumes) {
    if (!audio->device) {
        return;
    }
    applyVolumes(*audio, *volumes);
    audio->device->update();
}
}  // namespace

AudioPlugin::AudioPlugin(std::unique_ptr<AudioRegistry> registry, config::Settings settings)
    : _registry{std::move(registry)}, _settings{std::move(settings)} {}

std::string_view AudioPlugin::getName() const noexcept { return kName; }

void AudioPlugin::build(ecs::App& app) {
    app.addPlugin(plugins::transform::TransformPlugin{});
    ecs::Events& events = app.getWorld().getEvents();
    static_cast<void>(events.registerType<PlaySound>());
    static_cast<void>(events.registerType<PlayMusic>());
    static_cast<void>(events.registerType<StopMusic>());

    ecs::Resources& resources = app.getWorld().getResources();
    resources.insertNonSend(AudioSetup{.registry = std::move(_registry), .settings = std::move(_settings)});
    resources.insertNonSend(Audio{});
    if (!resources.has<AudioAssets>()) {
        resources.insert(AudioAssets{});
    }
    if (!resources.has<AudioVolumes>()) {
        resources.insert(AudioVolumes{});
    }
    static_cast<void>(app.getWorld().getComponents().registerType<AudioSource>());
    static_cast<void>(app.getWorld().getComponents().registerType<AudioListener>());

    app.addSystem(ecs::Phase::kStartup, std::string{kCreateSystem}, &createDevice).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kSoundsSystem}, &loadSounds).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kPlaySystem}, &playEvents)
        .after(std::string{kSoundsSystem})
        .after(std::string{plugins::transform::TransformPlugin::kPropagateSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kSourcesSystem}, &followSources)
        .after(std::string{kPlaySystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kUpdateSystem}, &updateDevice)
        .after(std::string{kSourcesSystem})
        .setSide(ecs::Side::kClient);
}
}  // namespace rtype::plugins::audio
