/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// The audio plugin, frame by frame: gameplay loads sounds by path, sends PlaySound / PlayMusic / StopMusic, and gives
// entities an AudioSource; the plugin turns that into calls on the audio backend the config names. Two backends are
// registered, as the executable would: "miniaudio", which plays the sounds of assets/ (maint.ogg, laser.mp3,
// laser-no.mp3), one step every 1.5 seconds; and "log", defined below, which plays nothing and prints every call it
// receives, so reading the output next to the script shows what each gameplay action costs the backend.
//
//   xmake run AudioBasics        hear it, through miniaudio
//   xmake run AudioBasics log    read the backend calls instead

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include "audio/miniaudio/Registration.hpp"
#include "plugins/asset/AssetId.hpp"
#include "plugins/audio/AudioAssets.hpp"
#include "plugins/audio/AudioConfig.hpp"
#include "plugins/audio/AudioEvents.hpp"
#include "plugins/audio/AudioListener.hpp"
#include "plugins/audio/AudioPlugin.hpp"
#include "plugins/audio/AudioRegistry.hpp"
#include "plugins/audio/AudioSource.hpp"
#include "plugins/audio/AudioTypes.hpp"
#include "plugins/audio/AudioVolumes.hpp"
#include "plugins/audio/IAudioDevice.hpp"
#include "plugins/config/Settings.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

using namespace rtype;
using namespace rtype::plugins::audio;
using rtype::plugins::asset::AssetId;
using rtype::plugins::transform::Transform;

namespace {
/// @return The name of a bus, for the log.
std::string_view busName(Bus bus) { return bus == Bus::kMusic ? "music" : "effects"; }

/// @brief A backend that plays nothing and prints every call: each sound gets the next id, each voice the next slot.
/// The plugin sets the volume and pan of every source each frame, so only the changes are printed.
class LogAudioDevice : public IAudioDevice {
  public:
    LogAudioDevice() = default;
    ~LogAudioDevice() override = default;
    LogAudioDevice(const LogAudioDevice& other) = delete;
    LogAudioDevice& operator=(const LogAudioDevice& other) = delete;
    LogAudioDevice(LogAudioDevice&& other) = delete;
    LogAudioDevice& operator=(LogAudioDevice&& other) = delete;

    SoundId createSound(std::span<const std::byte> encoded, const SoundDesc& desc) override {
        std::cout << std::format("  [device] createSound({} bytes{}) -> sound {}\n", encoded.size(),
                                 desc.stream ? ", streamed" : "", _nextSound);
        return SoundId{.value = _nextSound++};
    }

    SoundId createSound(const std::filesystem::path& path, const SoundDesc& desc) override {
        std::cout << std::format("  [device] createSound(\"{}\"{}) -> sound {}\n", path.generic_string(),
                                 desc.stream ? ", streamed" : "", _nextSound);
        return SoundId{.value = _nextSound++};
    }

    void destroySound(SoundId sound) override {
        std::cout << std::format("  [device] destroySound({})\n", sound.value);
    }

    VoiceId play(SoundId sound, const PlayParams& params) override {
        std::cout << std::format(
            "  [device] play(sound {}, bus {}, volume {:.2f}, pitch {:.2f}, pan {:+.2f}{}{}) -> voice {}\n",
            sound.value, busName(params.bus), params.volume, params.pitch, params.pan, params.loop ? ", loop" : "",
            params.fadeIn > 0.0F ? std::format(", fade in {}s", params.fadeIn) : "", _nextVoice);
        _volumes[_nextVoice] = params.volume;
        _pans[_nextVoice] = params.pan;
        return VoiceId{.index = _nextVoice++, .generation = 1};
    }

    void stop(VoiceId voice, float fadeOut) override {
        std::cout << std::format("  [device] stop(voice {}, fade out {}s)\n", voice.index, fadeOut);
    }

    void setPaused(VoiceId voice, bool paused) override {
        std::cout << std::format("  [device] setPaused(voice {}, {})\n", voice.index, paused);
    }

    void setVolume(VoiceId voice, float volume) override {
        if (changes(_volumes, voice, volume)) {
            std::cout << std::format("  [device] setVolume(voice {}, {:.2f})\n", voice.index, volume);
        }
    }

    void setPitch(VoiceId /*voice*/, float /*pitch*/) override {}  // Set every frame like the volume: not printed.

    void setPan(VoiceId voice, float pan) override {
        if (changes(_pans, voice, pan)) {
            std::cout << std::format("  [device] setPan(voice {}, {:+.2f})\n", voice.index, pan);
        }
    }

    [[nodiscard]] bool isPlaying(VoiceId /*voice*/) const override { return true; }

    void setMasterVolume(float volume) override {
        std::cout << std::format("  [device] setMasterVolume({:.2f})\n", volume);
    }

    void setBusVolume(Bus bus, float volume) override {
        std::cout << std::format("  [device] setBusVolume({}, {:.2f})\n", busName(bus), volume);
    }

    void update() override {}  // Once per frame: not printed.

  private:
    /// @return Whether @p value differs from the last one of @p voice in @p values, recording it.
    static bool changes(std::map<std::uint32_t, float>& values, VoiceId voice, float value) {
        const auto [known, inserted] = values.try_emplace(voice.index, value);
        if (inserted || known->second == value) {
            return false;
        }
        known->second = value;
        return true;
    }

    std::uint32_t _nextSound{1};              ///< The id of the next sound.
    std::uint32_t _nextVoice{0};              ///< The slot of the next voice.
    std::map<std::uint32_t, float> _volumes;  ///< The last volume of each voice.
    std::map<std::uint32_t, float> _pans;     ///< The last pan of each voice.
};

/// @brief The sounds of the game, loaded at startup.
struct GameSounds {
    AssetId music;    ///< The background track, streamed.
    AssetId misfire;  ///< A one-shot effect.
    AssetId laser;    ///< The ship's cannon, looped as an autofire.
};
RTYPE_ECS_RESOURCE(GameSounds, "example.GameSounds");

/// @brief The frame the script is at.
struct Script {
    int frame{0};  ///< Counted from 1 by the script system.
};
RTYPE_ECS_RESOURCE(Script, "example.Script");

/// @brief Marks the player's ship.
struct Ship {};
RTYPE_ECS_COMPONENT(Ship, "example.Ship");

/// @brief Startup: asks for the sounds (they load in this frame's PostUpdate), places the listener in the middle of an
/// 800-unit-wide screen, and spawns the ship on the left with its cannon firing.
void setup(ecs::ResMut<AudioAssets> sounds, ecs::ResMut<GameSounds> game, ecs::Commands& commands) {
    game->music = sounds->load("assets/maint.ogg", SoundDesc{.stream = true});
    game->misfire = sounds->load("assets/laser-no.mp3");
    game->laser = sounds->load("assets/laser.mp3");
    commands.spawn().add(Transform{.position = {400.0F, 300.0F}}).add(AudioListener{});
    commands.spawn()
        .add(Ship{})
        .add(Transform{.position = {200.0F, 300.0F}})
        .add(AudioSource{.sound = game->laser, .volume = 0.5F, .loop = true});
}

/// @brief Update: what gameplay does, one step per frame.
void script(ecs::ResMut<Script> script, ecs::Res<GameSounds> game, ecs::ResMut<AudioVolumes> volumes,
            ecs::EventWriter<PlaySound> playSounds, ecs::EventWriter<PlayMusic> playMusics,
            ecs::EventWriter<StopMusic> stopMusics, ecs::Query<Transform, ecs::With<Ship>> ships,
            ecs::Commands& commands) {
    switch (++script->frame) {
        case 1:
            std::cout << "  (the sounds load, then the cannon of the ship starts firing, panned left)\n";
            playMusics.send(PlayMusic{.sound = game->music, .crossfade = 2.0F});
            std::cout << "  (PlayMusic: the music starts in the same frame, fading in)\n";
            break;
        case 2:
            playSounds.send(PlaySound{.sound = game->misfire, .position = glm::vec2{700.0F, 300.0F}});
            std::cout << "  (PlaySound on the right of the screen)\n";
            break;
        case 3:
            for (auto [entity, transform] : ships) {
                transform->position.x = 800.0F;
            }
            std::cout << "  (the ship flies to the right edge: its cannon pans right)\n";
            break;
        case 4:
            volumes->music = 0.4F;
            std::cout << "  (a settings menu turns the music down)\n";
            break;
        case 5:
            for (auto [entity, ship] : ships) {
                commands.despawn(entity);
            }
            std::cout << "  (the ship is destroyed: its cannon stops)\n";
            break;
        case 6:
            stopMusics.send(StopMusic{.fadeOut = 1.5F});
            std::cout << "  (StopMusic)\n";
            break;
        default:
            break;
    }
}

/// @brief How long a step of the script lasts when it is heard: long enough to hear each sound.
constexpr std::chrono::milliseconds kHeardStep{1500};

/// @brief Runs the example through @p backend ("miniaudio" or "log"), printing each frame.
void run(const std::string& backend) {
    // The executable fills the registry: the real backends through their register function, and here the "log" device.
    auto registry = std::make_unique<AudioRegistry>();
    audio::miniaudio::registerAudio(*registry);
    registry->add(
        "log",
        [](const plugins::config::Settings& section, const AudioConfig& /*config*/) -> std::unique_ptr<IAudioDevice> {
            section.checkKeys({}, "log");
            return std::make_unique<LogAudioDevice>();
        });

    plugins::config::Settings settings;
    settings.set("audio.backend", backend);
    settings.set("audio.volume.music", 0.8);

    ecs::App app{ecs::RunMode::kStandalone};
    app.addPlugins(ecs::MinimalPlugins{}).addPlugin(AudioPlugin{std::move(registry), std::move(settings)});
    app.getWorld().getResources().insert(GameSounds{});
    app.getWorld().getResources().insert(Script{});
    app.addSystem(ecs::Phase::kStartup, "example.setup", &setup);
    app.addSystem(ecs::Phase::kUpdate, "example.script", &script);

    const bool heard = backend != "log";
    for (int frame = 1; frame <= 8; ++frame) {
        std::cout << std::format("== Frame {} ==\n", frame);
        if (frame == 1) {
            std::cout << "  (Startup creates the device with the volumes of the config)\n";
        }
        app.runFrame();
        if (heard) {
            std::this_thread::sleep_for(kHeardStep);  // The device mixes on its own thread meanwhile.
        }
    }
}
}  // namespace

int main(int argc, char** argv) {
    try {
        run(argc > 1 ? std::string{argv[1]} : std::string{"miniaudio"});
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
