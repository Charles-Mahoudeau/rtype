/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Measures what the physics of a room costs at 60 Hz, to know how many bodies a room supports and how many rooms a
// server holds. A room is what the server runs per game: an App in server mode with the physics plugin, ticked by
// hand. Its arena holds bullets (dynamic circles bouncing off the walls and the enemies, never off each other) and
// enemies (kinematic boxes patrolling up and down), one enemy for four bullets. Each scenario runs 100, 1 000 and
// 5 000 bodies per room, in 1, 4 and 8 rooms at once, one thread per room as on the server.
//
// For each scenario, it prints the cost of a fixed tick per room (the plugin's systems: body sync, Transform and
// Velocity sync, step, write-back, events), the part spent in Jolt's step, the cost per body, the 99th percentile
// tick, the spread between runs and how many such rooms fit on one core at 60 Hz. Ticks are summed up by their
// median, which a tick the OS preempted does not move: that keeps runs within a few percent of each other; the 99th
// percentile shows the slow ticks. Each scenario reports the median of its runs.
// Build in release mode: xmake f -m release --PhysicsBenchmark=y && xmake run PhysicsBenchmark
//
// Options: --steps N measured ticks per run (300), --warmup N ticks before measuring (60), --runs N (5), --quick for
// the 100 and 1 000 bodies scenarios in 1 and 4 rooms only.

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <iostream>
#include <iterator>
#include <latch>
#include <memory>
#include <numbers>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/Velocity.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/time/Time.hpp"

using namespace rtype::ecs;
using namespace rtype::plugins::physics;
using rtype::plugins::transform::Transform;
using Clock = std::chrono::steady_clock;

namespace {
constexpr float kFixedDelta = 1.0F / 60.0F;
constexpr double kBudgetMs = 1000.0 / 60.0;  // One tick at 60 Hz.
constexpr glm::vec2 kArena{1920.0F, 1080.0F};
constexpr float kWallThickness = 40.0F;
constexpr float kBulletRadius = 4.0F;
constexpr glm::vec2 kEnemySize{32.0F, 32.0F};
constexpr std::uint32_t kBulletsPerEnemy = 4;

// The collision categories: bullets hit walls and enemies, never each other; walls and enemies never meet.
constexpr std::uint32_t kWall = 1U << 0;
constexpr std::uint32_t kBullet = 1U << 1;
constexpr std::uint32_t kEnemy = 1U << 2;

struct Scenario {
    std::uint32_t bodies;
    std::uint32_t rooms;
};

struct Options {
    std::uint32_t steps = 300;
    std::uint32_t warmup = 60;
    std::uint32_t runs = 5;
    bool quick = false;
};

// What one room measured during one run.
struct RoomResult {
    double tickMs = 0.0;      // Median fixed tick.
    double stepMs = 0.0;      // Median time in PhysicsWorld::step().
    double slowTickMs = 0.0;  // 99th percentile fixed tick.
    bool droppedContacts = false;
};

// The time spent in each step, measured by two systems around "rtype.Physics.Step".
struct StepClock {
    Clock::time_point start;
    std::vector<double> stepsMs;
};
RTYPE_ECS_RESOURCE(StepClock, "bench.StepClock");

// FixedUpdate, right before the step.
void startStepClock(ResMut<StepClock> clock) { clock->start = Clock::now(); }

// FixedUpdate, right after the step.
void stopStepClock(ResMut<StepClock> clock) {
    clock->stepsMs.push_back(std::chrono::duration<double, std::milli>{Clock::now() - clock.get().start}.count());
}

// The value below which a fraction of the values lie, from 0 (the smallest) to 1 (the largest).
double percentileOf(std::vector<double> values, double fraction) {
    std::ranges::sort(values);
    const auto last = static_cast<double>(values.size() - 1);
    return values.at(static_cast<std::size_t>(std::lround(fraction * last)));
}

// FixedUpdate, before the Velocities drive the bodies: the enemies turn back at the top and bottom of the arena.
void patrolEnemies(Query<const Transform, Velocity, const Collider> bodies) {
    for (auto [entity, transform, velocity, collider] : bodies) {
        const float y = transform.position.y;
        const float speed = velocity.get().linear.y;
        if (collider.layer == kEnemy &&
            ((y < kArena.y * 0.1F && speed < 0.0F) || (y > kArena.y * 0.9F && speed > 0.0F))) {
            velocity->linear.y = -speed;
        }
    }
}

// Capacities that fit the room: Jolt drops the contacts beyond them, which the results flag.
PhysicsSettings settingsFor(std::uint32_t bodies) {
    return PhysicsSettings{.gravity = {0.0F, 0.0F},
                           .maxBodies = bodies + 8,
                           .maxBodyPairs = std::max(1024U, bodies * 4),
                           .maxContactConstraints = std::max(1024U, bodies * 2),
                           .tempAllocatorBytes = static_cast<std::size_t>(16 + (bodies / 256)) * 1024 * 1024};
}

// Spawns an entity with a Transform, a Collider and, when moving, a RigidBody and a Velocity.
void spawn(App& app, glm::vec2 position, const Collider& collider, const RigidBody* body = nullptr,
           glm::vec2 velocity = {0.0F, 0.0F}) {
    const Entity entity = app.getWorld().spawn();
    Components& components = app.getWorld().getComponents();
    components.add(entity, Transform{.position = position});
    components.add(entity, collider);
    if (body != nullptr) {
        components.add(entity, *body);
        components.add(entity, Velocity{.linear = velocity});
    }
}

// Builds a room: the arena's four walls, then the enemies on the right and the bullets everywhere, placed by a seeded
// generator so every run builds the same room.
std::unique_ptr<App> makeRoom(std::uint32_t bodies, std::uint32_t seed) {
    auto app = std::make_unique<App>(RunMode::kServer);
    app->addPlugin(PhysicsPlugin{settingsFor(bodies)});
    app->insertResource(Time{.fixedDelta = kFixedDelta});
    app->insertResource(StepClock{});
    app->addSystem(Phase::kFixedUpdate, "bench.patrolEnemies", &patrolEnemies)
        .after(std::string{PhysicsPlugin::kSyncBodiesSystem})
        .before(std::string{PhysicsPlugin::kSyncToPhysicsSystem});
    app->addSystem(Phase::kFixedUpdate, "bench.startStepClock", &startStepClock)
        .after(std::string{PhysicsPlugin::kSyncToPhysicsSystem})
        .before(std::string{PhysicsPlugin::kStepSystem});
    app->addSystem(Phase::kFixedUpdate, "bench.stopStepClock", &stopStepClock)
        .after(std::string{PhysicsPlugin::kStepSystem})
        .before(std::string{PhysicsPlugin::kSyncFromPhysicsSystem});

    const Collider horizontal{.shape = ShapeType::kBox,
                              .size = {kArena.x, kWallThickness},
                              .friction = 0.0F,
                              .layer = kWall,
                              .mask = kBullet};
    const Collider vertical{.shape = ShapeType::kBox,
                            .size = {kWallThickness, kArena.y},
                            .friction = 0.0F,
                            .layer = kWall,
                            .mask = kBullet};
    spawn(*app, {kArena.x / 2.0F, -kWallThickness / 2.0F}, horizontal);
    spawn(*app, {kArena.x / 2.0F, kArena.y + (kWallThickness / 2.0F)}, horizontal);
    spawn(*app, {-kWallThickness / 2.0F, kArena.y / 2.0F}, vertical);
    spawn(*app, {kArena.x + (kWallThickness / 2.0F), kArena.y / 2.0F}, vertical);

    std::mt19937 random{seed};
    std::uniform_real_distribution<float> unit{0.0F, 1.0F};
    const std::uint32_t enemies = bodies / (kBulletsPerEnemy + 1);
    const Collider enemy{.shape = ShapeType::kBox, .size = kEnemySize, .layer = kEnemy, .mask = kBullet};
    const RigidBody patrolling{.type = BodyType::kKinematic};
    for (std::uint32_t index = 0; index < enemies; ++index) {
        const glm::vec2 position{kArena.x * (0.6F + (0.35F * unit(random))), kArena.y * (0.1F + (0.8F * unit(random)))};
        spawn(*app, position, enemy, &patrolling, {0.0F, unit(random) < 0.5F ? -80.0F : 80.0F});
    }
    const Collider bullet{.shape = ShapeType::kCircle,
                          .radius = kBulletRadius,
                          .friction = 0.0F,
                          .restitution = 1.0F,
                          .layer = kBullet,
                          .mask = kWall | kEnemy};
    const RigidBody flying{.linearDamping = 0.0F, .gravityScale = 0.0F, .fixedRotation = true};
    for (std::uint32_t index = enemies; index < bodies; ++index) {
        const glm::vec2 position{kArena.x * (0.02F + (0.96F * unit(random))),
                                 kArena.y * (0.02F + (0.96F * unit(random)))};
        const float angle = 2.0F * std::numbers::pi_v<float> * unit(random);
        const float speed = 300.0F + (300.0F * unit(random));
        spawn(*app, position, bullet, &flying, glm::vec2{std::cos(angle), std::sin(angle)} * speed);
    }
    return app;
}

// Ticks a room through the warm-up, waits for the other rooms, then measures its ticks.
RoomResult runRoom(App& app, const Options& options, std::latch& ready) {
    for (std::uint32_t tick = 0; tick < options.warmup; ++tick) {
        app.getSchedules().run(Phase::kFixedUpdate);
    }
    app.getWorld().getResources().getMut<StepClock>()->stepsMs.clear();
    ready.arrive_and_wait();
    RoomResult result;
    std::vector<double> ticksMs;
    ticksMs.reserve(options.steps);
    for (std::uint32_t tick = 0; tick < options.steps; ++tick) {
        const Clock::time_point start = Clock::now();
        app.getSchedules().run(Phase::kFixedUpdate);
        ticksMs.push_back(std::chrono::duration<double, std::milli>{Clock::now() - start}.count());
        result.droppedContacts =
            result.droppedContacts || app.getWorld().getResources().require<PhysicsWorld>().didLastStepDropContacts();
    }
    result.tickMs = percentileOf(ticksMs, 0.5);
    result.slowTickMs = percentileOf(ticksMs, 0.99);
    result.stepMs = percentileOf(app.getWorld().getResources().require<StepClock>().stepsMs, 0.5);
    return result;
}

// One run of a scenario: builds its rooms, then ticks them together, one thread each.
std::vector<RoomResult> runScenario(const Scenario& scenario, const Options& options, std::uint32_t run) {
    std::vector<std::unique_ptr<App>> rooms;
    for (std::uint32_t room = 0; room < scenario.rooms; ++room) {
        rooms.push_back(makeRoom(scenario.bodies, (run * 1000U) + room));
    }
    std::vector<RoomResult> results(scenario.rooms);
    std::latch ready{static_cast<std::ptrdiff_t>(scenario.rooms)};
    std::vector<std::thread> threads;
    threads.reserve(scenario.rooms);
    for (std::uint32_t room = 0; room < scenario.rooms; ++room) {
        threads.emplace_back([&, room] { results.at(room) = runRoom(*rooms.at(room), options, ready); });
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
    return results;
}

// The mean of a field over the rooms of a run.
template <typename Field>
double meanOf(const std::vector<RoomResult>& rooms, Field field) {
    double sum = 0.0;
    for (const RoomResult& room : rooms) {
        sum += field(room);
    }
    return sum / static_cast<double>(rooms.size());
}

// Runs a scenario several times and prints its row.
void benchmark(const Scenario& scenario, const Options& options) {
    std::vector<double> ticks;
    std::vector<double> steps;
    std::vector<double> slow;
    bool dropped = false;
    for (std::uint32_t run = 0; run < options.runs; ++run) {
        const std::vector<RoomResult> rooms = runScenario(scenario, options, run);
        ticks.push_back(meanOf(rooms, [](const RoomResult& room) { return room.tickMs; }));
        steps.push_back(meanOf(rooms, [](const RoomResult& room) { return room.stepMs; }));
        slow.push_back(meanOf(rooms, [](const RoomResult& room) { return room.slowTickMs; }));
        for (const RoomResult& room : rooms) {
            dropped = dropped || room.droppedContacts;
        }
    }
    const double tick = percentileOf(ticks, 0.5);
    const auto [fastest, slowest] = std::ranges::minmax(ticks);
    std::cout << std::format(
        "| {:>6} | {:>5} | {:>9.3f} | {:>9.3f} | {:>8.3f} | {:>10.3f} | {:>6.1f}% | {:>13.0f} |{}\n", scenario.bodies,
        scenario.rooms, tick, percentileOf(steps, 0.5), tick * 1000.0 / scenario.bodies, percentileOf(slow, 0.5),
        (slowest - fastest) * 100.0 / tick, kBudgetMs / tick, dropped ? " contacts dropped" : "");
}

// Reads a count after an option; keeps the default when it is missing or not a positive number.
void readCount(std::span<char*>::iterator& option, std::span<char*>::iterator end, std::uint32_t& count) {
    if (std::next(option) == end) {
        return;
    }
    const std::string value{*++option};
    std::uint32_t parsed = 0;
    if (std::from_chars(value.c_str(), std::to_address(value.end()), parsed).ec == std::errc{} && parsed > 0) {
        count = parsed;
    }
}
}  // namespace

int main(int argc, char** argv) {
    Options options;
    const std::span<char*> arguments{argv, static_cast<std::size_t>(argc)};
    const std::span<char*> given = arguments.empty() ? arguments : arguments.subspan(1);  // Without the program.
    for (auto option = given.begin(); option != given.end(); ++option) {
        const std::string_view name{*option};
        if (name == "--steps") {
            readCount(option, given.end(), options.steps);
        } else if (name == "--warmup") {
            readCount(option, given.end(), options.warmup);
        } else if (name == "--runs") {
            readCount(option, given.end(), options.runs);
        } else if (name == "--quick") {
            options.quick = true;
        }
    }
    try {
#ifdef NDEBUG
        constexpr std::string_view kBuild = "release";
#else
        constexpr std::string_view kBuild = "debug: build in release mode for meaningful numbers";
#endif
        std::cout << std::format(
            "Physics benchmark: {} measured ticks after {} of warm-up, median of {} runs, {} "
            "hardware threads, {} build. Budget at 60 Hz: {:.2f} ms per tick.\n\n",
            options.steps, options.warmup, options.runs, std::thread::hardware_concurrency(), kBuild, kBudgetMs);
        std::cout << "| bodies | rooms | tick (ms) | step (ms) | us/body |  p99 (ms) | spread | rooms/core@60 |\n"
                  << "|-------:|------:|----------:|----------:|--------:|-----------:|-------:|--------------:|\n";
        for (const std::uint32_t bodies : {100U, 1000U, 5000U}) {
            for (const std::uint32_t rooms : {1U, 4U, 8U}) {
                if (!options.quick || (bodies <= 1000 && rooms <= 4)) {
                    benchmark(Scenario{.bodies = bodies, .rooms = rooms}, options);
                }
            }
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "PhysicsBenchmark: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "PhysicsBenchmark: unknown error\n";
    }
    return 1;
}
