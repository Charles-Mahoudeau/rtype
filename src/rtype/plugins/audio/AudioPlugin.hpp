/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioPlugin
*/

#pragma once

#include <memory>
#include <string_view>

#include "core/config/Settings.hpp"
#include "plugins/audio/AudioRegistry.hpp"
#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::plugins::audio {
/// @brief Sound for an App: starts the audio backend the configuration names, loads the sounds gameplay asks for, and
/// plays the PlaySound / PlayMusic / StopMusic events and the AudioSource components.
///
/// @details The registry comes filled by the executable (the only place that knows miniaudio), so this plugin depends
/// on no backend. It adds client systems only, so a dedicated server opens no device:
/// - Startup "rtype.audio.create": creates the device (AudioRegistry::createDevice()) into the main-thread resource
///   "rtype.Audio", and sets the AudioVolumes resource from the config.
/// - PostUpdate "rtype.audio.sounds": destroys the unloaded sounds and loads the queued ones of the AudioAssets
///   resource (updateSounds()), so a sound asked for during this frame's Update plays in this frame.
/// - PostUpdate "rtype.audio.play", after it: plays the PlaySound events, then handles StopMusic and PlayMusic (a
///   PlayMusic sent in the same frame as a StopMusic wins).
/// - PostUpdate "rtype.audio.sources", after it and after the transform propagation: starts the AudioSources whose
///   sound is loaded, restarts those whose sound changed, stops those removed or despawned, and updates the volume,
///   pitch and pan of the others.
/// - PostUpdate "rtype.audio.update", last: applies the AudioVolumes if they changed, then IAudioDevice::update().
///
/// Panning: pan = (x - listener x) / panDistance, clamped to [-1, 1], from the GlobalTransforms of the source and of
/// the AudioListener (their Transform in the frame they spawn, before they get a GlobalTransform); no listener, no
/// panning. It also inserts the AudioAssets and AudioVolumes resources (unless the game did), registers
/// AudioSource, AudioListener and the events, and adds the TransformPlugin (nothing happens if it was added first).
/// @code
/// auto audio = std::make_unique<rtype::plugins::audio::AudioRegistry>();
/// rtype::audio::miniaudio::registerAudio(*audio);
/// app.addPlugin(rtype::plugins::audio::AudioPlugin{std::move(audio), settings});
/// @endcode
class AudioPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.audio";                   ///< The plugin's name.
    static constexpr std::string_view kCreateSystem = "rtype.audio.create";    ///< Startup: creates the device.
    static constexpr std::string_view kSoundsSystem = "rtype.audio.sounds";    ///< PostUpdate: loads the sounds.
    static constexpr std::string_view kPlaySystem = "rtype.audio.play";        ///< PostUpdate: plays the events.
    static constexpr std::string_view kSourcesSystem = "rtype.audio.sources";  ///< PostUpdate: follows the sources.
    static constexpr std::string_view kUpdateSystem = "rtype.audio.update";    ///< PostUpdate: updates the device.

    /// @param registry The audio backends, registered by the executable.
    /// @param settings The configuration: `audio` and a section per backend.
    AudioPlugin(std::unique_ptr<AudioRegistry> registry, core::config::Settings settings);
    ~AudioPlugin() override = default;
    AudioPlugin(const AudioPlugin& other) = delete;
    AudioPlugin& operator=(const AudioPlugin& other) = delete;
    AudioPlugin(AudioPlugin&& other) noexcept = default;
    AudioPlugin& operator=(AudioPlugin&& other) noexcept = default;

    /// @return "rtype.audio".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Registers the components and events, inserts the resources and adds the systems. Opens nothing: the
    /// device is created by the Startup system.
    void build(ecs::App& app) override;

  private:
    std::unique_ptr<AudioRegistry> _registry;  ///< Handed to the Startup system by build().
    core::config::Settings _settings;          ///< Handed to the Startup system by build().
};
}  // namespace rtype::plugins::audio
