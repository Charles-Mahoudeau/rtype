/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Dynamic queries: the same queries, built at run time from component ids, for code that does not know the
// components at compile time (replication, scripts). Values are bytes, numbered by data term.

#include <cstddef>
#include <cstring>
#include <exception>
#include <format>
#include <iostream>
#include <span>
#include <vector>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/query/DynamicQuery.hpp"
#include "rtype/ecs/query/QueryBuilder.hpp"
#include "rtype/ecs/query/QueryIteration.hpp"
#include "rtype/ecs/query/QueryMatch.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/world/World.hpp"

using namespace rtype::ecs;

namespace {
struct Position {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(Position, "example.Position", x, y);

struct Velocity {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(Velocity, "example.Velocity", x, y);

/// @return The float stored at `offset` in `bytes`.
float readFloat(std::span<const std::byte> bytes, std::size_t offset) {
    float value = 0.F;
    std::memcpy(&value, bytes.subspan(offset, sizeof(float)).data(), sizeof(float));
    return value;
}

/// @brief Stores `value` at `offset` in `bytes`.
void writeFloat(std::span<std::byte> bytes, std::size_t offset, float value) {
    std::memcpy(bytes.subspan(offset, sizeof(float)).data(), &value, sizeof(float));
}

/// @brief Runs the example, printing each step.
void run() {
    World world;
    Components& components = world.getComponents();

    // A C++ component and one described at run time (as a script would), side by side.
    const ComponentId position = components.registerType<Position>();
    const ComponentId velocity = components.registerType<Velocity>();
    const ComponentId shield = *components.registerType(
        ComponentInfo{.type = {.name = "example.Shield",
                               .size = sizeof(float),
                               .alignment = alignof(float),
                               .fields = {{.name = "strength", .type = FieldType::kF32, .offset = 0}}}});

    const Entity ship = world.spawn(Position{.x = 0.F, .y = 0.F}, Velocity{.x = 1.F, .y = 2.F});
    std::vector<std::byte> shieldValue(sizeof(float));
    writeFloat(shieldValue, 0, 40.F);
    components.add(ship, shield, shieldValue);
    const Entity rock = world.spawn(Position{.x = 9.F, .y = 9.F});
    std::cout << std::format("ship #{} (position, velocity, shield), rock #{} (position)\n", ship.getIndex(),
                             rock.getIndex());

    std::cout << "\n== write(position).read(velocity): data terms 0 and 1 ==\n";
    const Result<DynamicQuery> movement = QueryBuilder{}.write(position).read(velocity).build(components);
    for (QueryMatch& match : movement->iter(0)) {
        const std::span<std::byte> bytes = match.getMut(0);     // write term: marks the value changed
        const std::span<const std::byte> speed = match.get(1);  // read term
        writeFloat(bytes, 0, readFloat(bytes, 0) + readFloat(speed, 0));
        writeFloat(bytes, sizeof(float), readFloat(bytes, sizeof(float)) + readFloat(speed, sizeof(float)));
        std::cout << std::format("  #{} moved to ({}, {})\n", match.getEntity().getIndex(), readFloat(bytes, 0),
                                 readFloat(bytes, sizeof(float)));
    }

    std::cout << "\n== read(position).optionalRead(shield): empty bytes when absent ==\n";
    const Result<DynamicQuery> armored = QueryBuilder{}.read(position).optionalRead(shield).build(components);
    for (const QueryMatch& match : armored->iter(0)) {
        const std::span<const std::byte> armor = match.get(1);
        std::cout << std::format("  #{} shield: {}\n", match.getEntity().getIndex(),
                                 armor.empty() ? "none" : std::format("{}", readFloat(armor, 0)));
    }

    std::cout << "\n== read(position).without(velocity): filters take no index ==\n";
    const Result<DynamicQuery> still = QueryBuilder{}.read(position).without(velocity).build(components);
    for (const QueryMatch& match : still->iter(0)) {
        std::cout << std::format("  #{} does not move\n", match.getEntity().getIndex());
    }

    std::cout << "\n== A read term refuses write access ==\n";
    const Result<DynamicQuery> readOnly = QueryBuilder{}.read(position).build(components);
    for (QueryMatch& match : readOnly->iter(0)) {
        std::cout << std::format("  #{} getMut(0) is empty: {}\n", match.getEntity().getIndex(),
                                 match.getMut(0).empty());
    }

    std::cout << "\n== A wrong query is an Error, not an exception ==\n";
    const Result<DynamicQuery> unknown = QueryBuilder{}.read(ComponentId{99}).build(components);
    if (!unknown) {
        std::cout << std::format("  {}\n", unknown.error().getMessage());
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
