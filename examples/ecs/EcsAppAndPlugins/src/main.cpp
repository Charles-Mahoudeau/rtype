/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// App and plugins: the App owns a world, its schedules and the loop. Plugins register components, resources and
// systems into it; a group adds several plugins; a plugin names the plugins it requires, built first; the App runs
// until a system sends AppExit.

#include <array>
#include <chrono>
#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/AppExit.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/ecs/app/PluginGroup.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/exceptions/PluginException.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"

using namespace rtype::ecs;

namespace {
struct Position {
    float x;
};
RTYPE_ECS_COMPONENT(Position, "example.Position", x);

struct Frames {
    int count;
};
RTYPE_ECS_RESOURCE(Frames, "example.Frames", count);

void spawnShip(Commands& commands) {
    static_cast<void>(commands.spawn().add(Position{.x = 0.F}));
    std::cout << "  Startup: spawned the ship\n";
}

void moveShip(Query<Position> ships) {
    for (auto [entity, position] : ships) {
        position->x += 1.F;
    }
}

/// @brief Counts frames and asks the App to stop after three.
void countFrames(ResMut<Frames> frames, EventWriter<AppExit> exit) {
    ++frames->count;
    std::cout << std::format("  frame {}\n", frames->count);
    if (frames->count == 3) {
        exit.send(AppExit{.code = 7});
    }
}

/// @brief A plugin: registers what the game needs. build() only registers; work goes in Startup systems.
class GamePlugin : public Plugin {
  public:
    GamePlugin() = default;
    ~GamePlugin() override = default;
    GamePlugin(const GamePlugin& other) = default;
    GamePlugin& operator=(const GamePlugin& other) = default;
    GamePlugin(GamePlugin&& other) noexcept = default;
    GamePlugin& operator=(GamePlugin&& other) noexcept = default;

    [[nodiscard]] std::string_view getName() const noexcept override { return "example.game"; }

    void build(App& app) override {
        std::cout << "  building example.game\n";
        app.insertResource(Frames{.count = 0});
        app.addSystem(Phase::kStartup, "example.spawnShip", &spawnShip);
        app.addSystem(Phase::kFixedUpdate, "example.moveShip", &moveShip);
        app.addSystem(Phase::kUpdate, "example.countFrames", &countFrames).setSide(Side::kBoth);
    }
};

/// @brief Another plugin, added by the group below.
class AudioPlugin : public Plugin {
  public:
    AudioPlugin() = default;
    ~AudioPlugin() override = default;
    AudioPlugin(const AudioPlugin& other) = default;
    AudioPlugin& operator=(const AudioPlugin& other) = default;
    AudioPlugin(AudioPlugin&& other) noexcept = default;
    AudioPlugin& operator=(AudioPlugin&& other) noexcept = default;

    [[nodiscard]] std::string_view getName() const noexcept override { return "example.audio"; }

    void build(App& /*app*/) override { std::cout << "  building example.audio\n"; }
};

/// @brief A group: adds several plugins at once, each skipped if already added.
class GamePlugins : public PluginGroup {
  public:
    GamePlugins() = default;
    ~GamePlugins() override = default;
    GamePlugins(const GamePlugins& other) = default;
    GamePlugins& operator=(const GamePlugins& other) = default;
    GamePlugins(GamePlugins&& other) noexcept = default;
    GamePlugins& operator=(GamePlugins&& other) noexcept = default;

    void build(App& app) override { app.addPlugin(GamePlugin{}).addPlugin(AudioPlugin{}); }
};

/// @brief A plugin the HUD requires: never added by hand, built from the App's plugin registry.
class TextPlugin : public Plugin {
  public:
    static constexpr std::string_view kName = "example.text";

    TextPlugin() = default;
    ~TextPlugin() override = default;
    TextPlugin(const TextPlugin& other) = default;
    TextPlugin& operator=(const TextPlugin& other) = default;
    TextPlugin(TextPlugin&& other) noexcept = default;
    TextPlugin& operator=(TextPlugin&& other) noexcept = default;

    [[nodiscard]] std::string_view getName() const noexcept override { return kName; }

    void build(App& /*app*/) override { std::cout << "  building example.text\n"; }
};

/// @brief A plugin declaring what it requires, by name: the App builds those first.
class HudPlugin : public Plugin {
  public:
    static constexpr std::array<std::string_view, 2> kDependencies{"example.game", TextPlugin::kName};

    HudPlugin() = default;
    ~HudPlugin() override = default;
    HudPlugin(const HudPlugin& other) = default;
    HudPlugin& operator=(const HudPlugin& other) = default;
    HudPlugin(HudPlugin&& other) noexcept = default;
    HudPlugin& operator=(HudPlugin&& other) noexcept = default;

    [[nodiscard]] std::string_view getName() const noexcept override { return "example.hud"; }
    [[nodiscard]] std::span<const std::string_view> getDependencies() const noexcept override { return kDependencies; }

    void build(App& /*app*/) override { std::cout << "  building example.hud\n"; }
};

/// @brief A plugin requiring one nobody added or registered.
class NetworkPlugin : public Plugin {
  public:
    static constexpr std::array<std::string_view, 1> kDependencies{"example.socket"};

    NetworkPlugin() = default;
    ~NetworkPlugin() override = default;
    NetworkPlugin(const NetworkPlugin& other) = default;
    NetworkPlugin& operator=(const NetworkPlugin& other) = default;
    NetworkPlugin(NetworkPlugin&& other) noexcept = default;
    NetworkPlugin& operator=(NetworkPlugin&& other) noexcept = default;

    [[nodiscard]] std::string_view getName() const noexcept override { return "example.network"; }
    [[nodiscard]] std::span<const std::string_view> getDependencies() const noexcept override { return kDependencies; }

    void build(App& /*app*/) override { std::cout << "  building example.network\n"; }
};

/// @brief Runs the example, printing each step.
void run() {
    App app{RunMode::kStandalone};  // one world, its schedules, one run mode

    std::cout << "== Adding plugins: built at once, skipped by name the second time ==\n";
    app.addPlugin(TimePlugin::manual(std::chrono::milliseconds{20}))  // deterministic time for the example
        .addPlugins(MinimalPlugins{})                                 // its TimePlugin is skipped: already added
        .addPlugins(GamePlugins{})
        .addPlugin(GamePlugin{});  // already added through the group: ignored
    std::cout << std::format("  has example.game? {}; has rtype.time? {}\n", app.hasPlugin("example.game"),
                             app.hasPlugin(TimePlugin::kName));

    std::cout << "\n== Dependencies: example.hud requires example.game (added) and example.text (registered) ==\n";
    app.getPluginRegistry().add(std::string{TextPlugin::kName}, [] { return std::make_unique<TextPlugin>(); });
    app.addPlugin(HudPlugin{});  // builds example.text first, with a warning on stderr
    std::cout << std::format("  has example.text? {}\n", app.hasPlugin(TextPlugin::kName));

    std::cout << "\n== A dependency neither added nor registered is refused ==\n";
    try {
        app.addPlugin(NetworkPlugin{});
    } catch (const exceptions::PluginException& error) {
        std::cout << std::format("  refused: {}\n", error.what());
    }
    std::cout << std::format("  has example.network? {}\n", app.hasPlugin("example.network"));

    std::cout << "\n== run(): Startup once, then frames until a system sends AppExit ==\n";
    const int code = app.run();
    std::cout << std::format("  run() returned {}; the world still holds {} entity\n", code,
                             app.getWorld().getEntityCount());
}
}  // namespace

int main() {
    try {
        run();
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
