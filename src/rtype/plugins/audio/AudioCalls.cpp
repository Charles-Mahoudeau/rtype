/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioCalls
*/

#include "plugins/audio/AudioCalls.hpp"

#include <cmath>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>

#include "plugins/audio/AudioAssets.hpp"
#include "plugins/audio/AudioEvents.hpp"
#include "plugins/audio/exceptions/AudioCallException.hpp"
#include "rtype/contract/audiodevice/AudioTypes.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

namespace rtype::plugins::audio {
namespace {
/// @return The bus named @p name.
/// @throws exceptions::AudioCallException If no bus has this name.
[[nodiscard]] contract::audiodevice::Bus requireBus(const std::string_view name) {
    if (name == "effects") {
        return contract::audiodevice::Bus::kEffects;
    }
    if (name == "music") {
        return contract::audiodevice::Bus::kMusic;
    }
    throw exceptions::AudioCallException{std::format("Unknown bus '{}' (expected 'effects' or 'music')", name)};
}

SoundResult loadSound(ecs::ResMut<AudioAssets> sounds, const LoadSoundArguments& arguments) {
    const contract::audiodevice::SoundDesc desc{.stream = arguments.stream};
    return SoundResult{.sound = sounds->load(std::filesystem::path{arguments.path.view()}, desc)};
}

void playSound(ecs::EventWriter<PlaySound> sounds, const PlaySoundArguments& arguments) {
    PlaySound sound{
        .sound = arguments.sound,
        .bus = requireBus(arguments.bus),
        .volume = arguments.volume,
        .pitch = arguments.pitch,
    };
    if (!std::isnan(arguments.position.x) && !std::isnan(arguments.position.y)) {
        sound.position = arguments.position;
    }
    sounds.send(sound);
}
}  // namespace

void addAudioCalls(ecs::App& app) {
    app.addCall(std::string{kLoadSoundCall}, &loadSound);
    app.addCall(std::string{kPlaySoundCall}, &playSound);
}

}  // namespace rtype::plugins::audio
