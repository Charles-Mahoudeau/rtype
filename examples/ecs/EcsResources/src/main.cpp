/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Resources: single values owned by the world (the score, the settings, the window), read with Res<T> and written
// with ResMut<T> in systems. NonSend<T> marks a resource that must stay on the main thread.

#include <exception>
#include <format>
#include <iostream>
#include <memory>

#include "rtype/ecs/exceptions/MissingResourceException.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/ecs/world/World.hpp"

using namespace rtype::ecs;

namespace {
struct Score {
    int value;
};
RTYPE_ECS_RESOURCE(Score, "example.Score", value);

struct Settings {
    int pointsPerKill;
};
RTYPE_ECS_RESOURCE(Settings, "example.Settings", pointsPerKill);

/// @brief A main-thread-only resource (a window handle would be one): it may even be move-only.
struct Window {
    std::unique_ptr<int> handle;  ///< Stands for the platform's window.
};
RTYPE_ECS_RESOURCE(Window, "example.Window");

/// @brief A resource nobody inserted.
struct Leaderboard {
    int best;
};
RTYPE_ECS_RESOURCE(Leaderboard, "example.Leaderboard", best);

/// @brief Reads the settings, writes the score.
void scoreAKill(ResMut<Score> score, Res<Settings> settings) {
    score->value += settings->pointsPerKill;
    std::cout << std::format("  score is now {} (settings changed since last run? {})\n", score->value,
                             settings.isChanged());
}

/// @brief Uses the main-thread resource.
void drawWindow(NonSendMut<Window> window) { std::cout << std::format("  drawing in window {}\n", *window->handle); }

/// @brief Needs a resource that is missing.
void readLeaderboard(Res<Leaderboard> leaderboard) { std::cout << leaderboard->best << "\n"; }

/// @brief Runs the example, printing each step.
void run() {
    World world;
    Resources& resources = world.getResources();

    std::cout << "== Inserting and reading ==\n";
    resources.insert(Score{.value = 0});
    resources.insert(Settings{.pointsPerKill = 100});
    std::cout << std::format("score {}, points per kill {}\n", resources.require<Score>().value,
                             resources.get<Settings>()->pointsPerKill);

    std::cout << "\n== In systems: Res reads, ResMut writes ==\n";
    System kill{world, "example.scoreAKill", &scoreAKill};
    kill.run();
    kill.run();

    std::cout << "\n== Replacing a resource marks it changed ==\n";
    resources.insert(Settings{.pointsPerKill = 250});
    kill.run();

    std::cout << "\n== A main-thread resource ==\n";
    resources.insertNonSend(Window{.handle = std::make_unique<int>(42)});
    System draw{world, "example.drawWindow", &drawWindow};
    std::cout << std::format("drawWindow must run on the main thread? {}\n", draw.getAccess().isMainThread());
    draw.run();

    std::cout << "\n== A missing resource ==\n";
    std::cout << std::format("get<Leaderboard>() -> {}\n",
                             resources.get<Leaderboard>() == nullptr ? "nullptr" : "a value");
    System leaderboard{world, "example.readLeaderboard", &readLeaderboard};
    try {
        leaderboard.run();
    } catch (const exceptions::MissingResourceException& error) {
        std::cout << std::format("run() threw: {}\n", error.what());
    }
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
