/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Events: messages systems send to each other (a hit, a window closing). An event is readable during the update it
// was sent in and the next one; each reader keeps a cursor, so it reads each event once.

#include <exception>
#include <format>
#include <iostream>

#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/ecs/world/World.hpp"

using namespace rtype::ecs;

namespace {
struct Hit {
    int damage;
};
RTYPE_ECS_EVENT(Hit, "example.Hit", damage);

struct DamageTaken {
    int total;
};
RTYPE_ECS_RESOURCE(DamageTaken, "example.DamageTaken", total);

/// @brief Sends two hits.
void shoot(EventWriter<Hit> hits) {
    hits.send(Hit{.damage = 10});
    hits.send(Hit{.damage = 25});
}

/// @brief Reads the hits it has not read yet.
void takeHits(EventReader<Hit> hits, ResMut<DamageTaken> damage) {
    for (const Hit& hit : hits) {
        damage->total += hit.damage;
    }
    std::cout << std::format("  read {} hits, total damage {}\n", hits.size(), damage->total);
}

/// @brief Runs the example, printing each step.
void run() {
    World world;
    Events& events = world.getEvents();

    std::cout << "== Sending and reading with a cursor ==\n";
    events.send(Hit{.damage = 5});
    EventCursor cursor;
    for (const Hit& hit : events.read<Hit>(cursor)) {
        std::cout << std::format("  hit for {}\n", hit.damage);
    }
    std::cout << std::format("  reading again with the same cursor: {} events\n", events.read<Hit>(cursor).size());

    std::cout << "\n== An event lives for its update and the next one ==\n";
    EventCursor late;  // a reader that has not read anything yet
    events.update<Hit>();
    std::cout << std::format("  after 1 update: {} kept, a late reader sees {}\n", events.getCount<Hit>(),
                             events.read<Hit>(late).size());
    events.update<Hit>();
    std::cout << std::format("  after 2 updates: {} kept\n", events.getCount<Hit>());

    std::cout << "\n== In systems: EventWriter and EventReader ==\n";
    world.getResources().insert(DamageTaken{.total = 0});
    System shooter{world, "example.shoot", &shoot};
    System target{world, "example.takeHits", &takeHits};
    shooter.run();
    target.run();
    target.run();  // nothing new: its cursor is past the two hits
    // In an App, the schedules move every queue forward once per frame (once per fixed tick for the events
    // written in FixedUpdate), so nobody calls update() by hand.
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
