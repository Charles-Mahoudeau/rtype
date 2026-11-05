/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// A splash screen and nothing else, the way an engine signs its games: luau.png zooms in over a background of its own
// colour (the curve is in game/scripts/all.luau), then wafprout.mp3 plays at a random speed. The moment it ends, the
// window closes and the program reports the worst news it knows.
//   xmake f "--ethan_game(bestoftheworld)=y" && xmake run "ethan_game(bestoftheworld)"

#include <cstdlib>
#include <exception>
#include <glm/ext/vector_float2.hpp>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <utility>

#include "audio/miniaudio/Registration.hpp"
#include "core/asset/AssetId.hpp"
#include "core/config/Settings.hpp"
#include "engine/scripting/SettingsLoader.hpp"
#include "platform/glfw/Registration.hpp"
#include "plugins/audio/AudioAssets.hpp"
#include "plugins/audio/AudioPlugin.hpp"
#include "plugins/audio/AudioRegistry.hpp"
#include "plugins/audio/AudioResources.hpp"
#include "plugins/audio/AudioTypes.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/BackendResources.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/SpriteRenderer.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/transform/Transform.hpp"
#include "render/vulkan/Registration.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/AppExit.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/scripting/plugin/ScriptingPlugin.hpp"

using namespace rtype;

namespace {
constexpr std::string_view kGame = "examples/games/ethan_game(bestoftheworld)/game";
constexpr float kWidth = 800.0F;   // As in project.luau.
constexpr float kHeight = 600.0F;  // As in project.luau.
constexpr glm::vec2 kImageSize{1024.0F, 765.0F};
constexpr float kFullSize = 0.7F;  // The image's scale once zoomed in: all.luau gives a share of it.
constexpr float kMinSoundSpeed = 0.5F;
constexpr float kMaxSoundSpeed = 3.0F;
// The image's background, 28/255 in sRGB: the clear colour is written as it is, in linear values.
constexpr plugins::backend::graphics::Color kBackground{0.0116F, 0.0116F, 0.0116F, 1.0F};
}  // namespace

// The splash screen: all.luau moves it forward, the systems below show it.
struct Splash {
    float pace{0.0F};      // 0 until the image is loaded, then 1: all.luau multiplies the time by it.
    float elapsed{0.0F};   // Seconds since it started.
    float zoom{0.0F};      // Share of the full size, 0 to 1.
    bool finished{false};  // The zoom is over.
};
RTYPE_ECS_COMPONENT(Splash, "game.Splash", pace, elapsed, zoom, finished);

// The sound the splash screen ends with.
struct Jingle {
    core::asset::AssetId sound;
    plugins::audio::VoiceId voice;
    bool started{false};
};
RTYPE_ECS_RESOURCE(Jingle, "game.Jingle");

namespace {
// Reports the worst news and leaves at once: no shutdown, so the window goes with the process, the instant it is said.
[[noreturn]] void handleSuccess() {
    std::cout << "Smegmentation fault (core dumped)" << std::endl;
    std::_Exit(139);
}

// A new speed for the sound at every launch.
float randomSoundSpeed() {
    std::random_device device;
    return std::uniform_real_distribution<float>{kMinSoundSpeed, kMaxSoundSpeed}(device);
}

// Startup: loads the image and the sound, and spawns the splash screen.
void spawnSplash(ecs::ResMut<plugins::backend::TextureAssets> textures, ecs::ResMut<plugins::audio::AudioAssets> sounds,
                 ecs::ResMut<Jingle> jingle, ecs::Commands& commands) {
    const std::string game{kGame};
    jingle->sound = sounds->load(game + "/assets/wafprout.mp3");
    commands.spawn()
        .add(plugins::transform::Transform{.position = {kWidth / 2.0F, kHeight / 2.0F}, .scale = glm::vec2{0.0F}})
        .add(plugins::backend::SpriteRenderer{.texture = textures->load(game + "/assets/luau.png"),
                                              .source = {{0.0F, 0.0F}, kImageSize},
                                              .origin = kImageSize / 2.0F})
        .add(Splash{});
}

// Update: starts the zoom once the image is loaded, and shows what all.luau computed.
void applyZoom(ecs::Res<plugins::backend::TextureAssets> textures,
               ecs::Query<plugins::transform::Transform, Splash, const plugins::backend::SpriteRenderer> splashes) {
    for (auto [entity, transform, splash, sprite] : splashes) {
        splash->pace = textures->getTexture(sprite.texture) ? 1.0F : 0.0F;
        transform->scale = glm::vec2{splash->zoom * kFullSize};
    }
}

// Update: once the zoom is over, plays the sound, and leaves the moment it ends.
void playJingle(ecs::NonSendMut<plugins::audio::Audio> audio, ecs::Res<plugins::audio::AudioAssets> sounds,
                ecs::ResMut<Jingle> jingle, ecs::Query<const Splash> splashes, ecs::EventWriter<ecs::AppExit> exit) {
    if (audio->device == nullptr) {
        return;
    }
    if (jingle->started) {
        if (!audio->device->isPlaying(jingle->voice)) {
            handleSuccess();
        }
        return;
    }
    const plugins::audio::AudioAssets::Entry* sound = sounds->get(jingle->sound);
    if (sound != nullptr && sound->state == plugins::audio::SoundState::kFailed) {
        std::cerr << "Error: " << sound->error << '\n';
        exit.send(ecs::AppExit{.code = 1});
        return;
    }
    if (sound == nullptr || sound->state != plugins::audio::SoundState::kLoaded) {
        return;
    }
    for (auto [entity, splash] : splashes) {
        if (splash.finished) {
            jingle->voice = audio->device->play(sound->sound, plugins::audio::PlayParams{.pitch = randomSoundSpeed()});
            jingle->started = true;
        }
    }
}

// The window, the renderer and the sound.
core::config::Settings makeSettings() {
    engine::scripting::SettingsLoader settingsLoader;
    settingsLoader.setFlag("isClient", true);
    settingsLoader.setFlag("isServer", false);
#ifdef NDEBUG
    settingsLoader.setFlag("isDebug", false);
#else
    settingsLoader.setFlag("isDebug", true);
#endif
    settingsLoader.load(std::string{kGame} + "/project.luau");
    return settingsLoader.build();
}
}  // namespace

int main() {
    try {
        auto registry = std::make_unique<plugins::backend::BackendRegistry>();
        platform::glfw::registerPlatform(*registry);
        render::vulkan::registerRenderer(*registry);
        auto audioRegistry = std::make_unique<plugins::audio::AudioRegistry>();
        audio::miniaudio::registerAudio(*audioRegistry);
        const core::config::Settings settings{makeSettings()};

        ecs::App app;
        app.addPlugins(ecs::MinimalPlugins{})
            .addPlugin(plugins::backend::BackendPlugin{std::move(registry), settings})
            .addPlugin(plugins::audio::AudioPlugin{std::move(audioRegistry), settings});
        app.insertResource(plugins::backend::ClearColor{.value = kBackground});
        app.insertResource(Jingle{});
        static_cast<void>(app.getWorld().getComponents().registerType<Splash>());
        app.addPlugin(scripting::ScriptingPlugin{
            {.directory = std::string{kGame} + "/scripts", .options = {.isClient = true, .isServer = true}}});
        app.addSystem(ecs::Phase::kStartup, "game.spawnSplash", &spawnSplash).setSide(ecs::Side::kClient);
        app.addSystem(ecs::Phase::kUpdate, "game.applyZoom", &applyZoom).after("game.zoom").setSide(ecs::Side::kClient);
        app.addSystem(ecs::Phase::kUpdate, "game.playJingle", &playJingle)
            .after("game.applyZoom")
            .setSide(ecs::Side::kClient);
        app.run();
        handleSuccess();
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
    }
    return 1;
}
