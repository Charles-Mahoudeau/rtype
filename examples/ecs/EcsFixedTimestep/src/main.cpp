/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Fixed timestep: gameplay runs in FixedUpdate at a fixed rate (60 ticks per second by default), whatever the frame
// rate. A frame may run FixedUpdate zero, one or several times; a cap stops a slow machine from falling behind.

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <format>
#include <iostream>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/Time.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"

using namespace rtype::ecs;

namespace {
/// @brief Feeds `frames` frames of `frame` to a 60 Hz timestep and prints the ticks they give.
void simulate(std::chrono::milliseconds frame, int frames) {
    FixedTimestep timestep;  // 60 ticks per second, at most 5 per frame
    std::uint32_t total = 0;
    std::uint32_t most = 0;
    for (int index = 0; index < frames; ++index) {
        const std::uint32_t ticks = timestep.advance(frame);
        total += ticks;
        most = std::max(most, ticks);
    }
    std::cout << std::format("  {} frames of {} ms: {} ticks in all, at most {} in one frame\n", frames, frame.count(),
                             total, most);
}

/// @brief FixedUpdate: sees the tick it runs in.
void tickGameplay(Res<Time> time) {
    std::cout << std::format("    FixedUpdate: tick {}, fixedDelta {:.4f} s\n", time->tick, time->fixedDelta);
}

/// @brief Update: sees the frame's duration and how far it is into the next tick.
void showFrame(Res<Time> time) {
    std::cout << std::format("    Update: delta {:.3f} s, alpha {:.2f}\n", time->delta, time->alpha);
}

/// @brief Runs the example, printing each step.
void run() {
    std::cout << "== One second of frames, whatever their length ==\n";
    simulate(std::chrono::milliseconds{5}, 200);  // fast frames: a tick every 3 or 4 frames
    simulate(std::chrono::milliseconds{16}, 60);  // almost one tick per frame
    simulate(std::chrono::milliseconds{50}, 20);  // slow frames: 3 ticks each

    std::cout << "\n== The cap: a one-second frame runs 5 ticks and drops the rest ==\n";
    FixedTimestep timestep;
    std::cout << std::format("  advance(1 s) -> {} ticks\n", timestep.advance(std::chrono::seconds{1}));

    std::cout << "\n== In an App: the time plugin keeps the Time resource up to date ==\n";
    App app;
    // Every frame lasts exactly 25 ms here (deterministic); without a TimePlugin, the App adds the real clock.
    app.addPlugin(TimePlugin::manual(std::chrono::milliseconds{25}));
    app.addSystem(Phase::kFixedUpdate, "example.tickGameplay", &tickGameplay);
    app.addSystem(Phase::kUpdate, "example.showFrame", &showFrame);
    for (int frame = 1; frame <= 3; ++frame) {
        std::cout << std::format("  frame {}:\n", frame);
        app.runFrame();
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
