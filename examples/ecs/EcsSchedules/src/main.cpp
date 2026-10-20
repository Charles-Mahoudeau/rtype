/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Schedules: a world's systems, one schedule per phase. Inside a phase systems run by name unless before / after
// order them; two systems touching the same data with no order are refused. runFrame() runs a whole frame.

#include <chrono>
#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <string_view>

#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/exceptions/ScheduleException.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/schedule/Schedules.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/TimeClock.hpp"
#include "rtype/ecs/world/World.hpp"

using namespace rtype::ecs;

namespace {
struct Health {
    int value;
};
RTYPE_ECS_COMPONENT(Health, "example.Health", value);

void noop() {}
void writeHealth(Query<Health> /*healths*/) {}

void startup() { std::cout << "  Startup\n"; }
void preUpdate() { std::cout << "  PreUpdate\n"; }
void fixedUpdate() { std::cout << "  FixedUpdate\n"; }
void update() { std::cout << "  Update\n"; }
void postUpdate() { std::cout << "  PostUpdate\n"; }
void render() { std::cout << "  Render\n"; }
void serverOnly() { std::cout << "  (server system)\n"; }
void clientOnly() { std::cout << "  (client system)\n"; }

/// @return The run mode's name.
std::string_view modeName(RunMode mode) {
    switch (mode) {
        case RunMode::kServer:
            return "server";
        case RunMode::kClient:
            return "client";
        case RunMode::kStandalone:
            return "standalone";
    }
    return "?";
}

/// @brief Prints the order of a phase, as of the last build.
void printOrder(const Schedules& schedules, Phase phase) {
    std::cout << std::format("  {}:", toString(phase));
    for (const std::string_view name : schedules.getOrder(phase)) {
        std::cout << " " << name;
    }
    std::cout << "\n";
}

/// @brief Runs the example, printing each step.
void run() {
    std::cout << "== Inside a phase: by name, unless ordered ==\n";
    {
        World world;
        Schedules schedules{world, RunMode::kStandalone};
        schedules.addSystem(Phase::kFixedUpdate, "game.damage", &noop).after("game.collision");
        schedules.addSystem(Phase::kFixedUpdate, "game.collision", &noop).after("game.movement");
        schedules.addSystem(Phase::kFixedUpdate, "game.movement", &noop);
        schedules.addSystem(Phase::kFixedUpdate, "game.audio", &noop);  // unordered: by name
        schedules.build();
        printOrder(schedules, Phase::kFixedUpdate);
    }

    std::cout << "\n== Two writers with no order are refused ==\n";
    {
        World world;
        Schedules schedules{world, RunMode::kStandalone};
        schedules.addSystem(Phase::kFixedUpdate, "game.damage", &writeHealth);
        schedules.addSystem(Phase::kFixedUpdate, "game.regen", &writeHealth);
        try {
            schedules.build();
        } catch (const exceptions::ScheduleException& error) {
            std::cout << std::format("  {}\n", error.what());
        }
        std::cout << "  ...and accepted once ordered:\n";
        Schedules ordered{world, RunMode::kStandalone};
        ordered.addSystem(Phase::kFixedUpdate, "game.damage", &writeHealth);
        ordered.addSystem(Phase::kFixedUpdate, "game.regen", &writeHealth).after("game.damage");
        ordered.build();
        printOrder(ordered, Phase::kFixedUpdate);
    }

    std::cout << "\n== A frame: Startup once, then the phases, FixedUpdate once per fixed tick ==\n";
    {
        World world;
        Schedules schedules{world, RunMode::kStandalone};
        schedules.addSystem(Phase::kStartup, "trace.startup", &startup);
        schedules.addSystem(Phase::kPreUpdate, "trace.preUpdate", &preUpdate);
        schedules.addSystem(Phase::kFixedUpdate, "trace.fixedUpdate", &fixedUpdate);
        schedules.addSystem(Phase::kUpdate, "trace.update", &update);
        schedules.addSystem(Phase::kPostUpdate, "trace.postUpdate", &postUpdate);
        schedules.addSystem(Phase::kRender, "trace.render", &render);
        // The clock decides how many fixed ticks a frame has: here every frame lasts 40 ms, 2.4 ticks at 60 Hz.
        schedules.setClock(std::make_unique<TimeClock>(FixedTimestep{}, std::chrono::milliseconds{40}));
        std::cout << " frame 1:\n";
        schedules.runFrame();
        std::cout << " frame 2:\n";
        schedules.runFrame();
    }

    std::cout << "\n== The run mode decides which sides run ==\n";
    for (const RunMode mode : {RunMode::kServer, RunMode::kClient, RunMode::kStandalone}) {
        World world;
        Schedules schedules{world, mode};
        schedules.addSystem(Phase::kUpdate, "example.server", &serverOnly);  // Side::kServer by default
        schedules.addSystem(Phase::kUpdate, "example.client", &clientOnly).setSide(Side::kClient);
        std::cout << std::format(" {}:\n", modeName(mode));
        schedules.run(Phase::kUpdate);
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
