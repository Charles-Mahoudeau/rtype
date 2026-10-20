/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Change detection: every component value records when it was added and last written. A system's Added<T> and
// Changed<T> filters report what happened since that system last ran, and only real writes count.

#include <exception>
#include <format>
#include <iostream>
#include <optional>

#include "rtype/ecs/change/ComponentTicks.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/ecs/world/World.hpp"

using namespace rtype::ecs;

namespace {
struct Health {
    int value;
};
RTYPE_ECS_COMPONENT(Health, "example.Health", value);

/// @brief Lists the entities whose Health was added or written since this system last ran.
void reportChanged(Query<const Health, Changed<Health>> query) {
    int count = 0;
    for (auto [entity, health] : query) {
        std::cout << std::format("  changed: #{} (health {})\n", entity.getIndex(), health.value);
        ++count;
    }
    if (count == 0) {
        std::cout << "  nothing changed\n";
    }
}

/// @brief Lists the entities whose Health was added since this system last ran.
void reportAdded(Query<const Health, Added<Health>> query) {
    int count = 0;
    for (auto [entity, health] : query) {
        std::cout << std::format("  added: #{} (health {})\n", entity.getIndex(), health.value);
        ++count;
    }
    if (count == 0) {
        std::cout << "  nothing added\n";
    }
}

/// @brief Runs the example, printing each step.
void run() {
    World world;
    Components& components = world.getComponents();
    const Entity ship = world.spawn(Health{.value = 100});
    const Entity enemy = world.spawn(Health{.value = 30});
    std::cout << std::format("ship #{}, enemy #{}\n", ship.getIndex(), enemy.getIndex());

    System changed{world, "example.reportChanged", &reportChanged};
    System added{world, "example.reportAdded", &reportAdded};

    std::cout << "\n== A system's first run sees everything as new ==\n";
    changed.run();
    added.run();

    std::cout << "\n== Its next run only reports what happened since ==\n";
    changed.run();

    std::cout << "\n== Writing through Mut (-> or *) marks the value ==\n";
    components.getMut<Health>(enemy)->value -= 10;
    changed.run();

    std::cout << "\n== Reading through Mut::get() does not ==\n";
    const Mut<Health> health = components.getMut<Health>(ship);
    std::cout << std::format("  read ship health: {}\n", health.get().value);
    changed.run();

    std::cout << "\n== Adding a component counts as added and changed ==\n";
    const Entity bullet = world.spawn();
    components.add(bullet, Health{.value = 1});
    added.run();
    changed.run();

    std::cout << "\n== The ticks behind it ==\n";
    if (const std::optional<ComponentTicks> ticks = components.getTicks<Health>(enemy)) {
        std::cout << std::format("  enemy Health: added at tick {}, changed at tick {}; world tick {}\n", ticks->added,
                                 ticks->changed, world.getChangeTick());
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
