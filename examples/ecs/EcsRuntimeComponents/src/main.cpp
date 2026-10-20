/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Runtime components: a component described at run time, with no C++ type behind it, as the Luau bridge registers
// the components a script declares. Its values are bytes, read and written at the offsets of its fields.

#include <cstddef>
#include <cstring>
#include <exception>
#include <format>
#include <iostream>
#include <optional>
#include <span>
#include <vector>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/world/World.hpp"

using namespace rtype::ecs;

namespace {
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

/// @brief Prints every f32 field of `entity`'s component `id`, reading the offsets from the registry: generic code
/// (the network, the Luau bridge) never hard-codes them.
void printFields(const Components& components, Entity entity, ComponentId id) {
    const ComponentInfo* info = components.getRegistry().findById(id);
    const std::span<const std::byte> bytes = components.get(entity, id);
    for (const FieldInfo& field : info->type.fields) {
        if (field.type == FieldType::kF32) {
            std::cout << std::format("  {}.{} = {}\n", info->type.name, field.name, readFloat(bytes, field.offset));
        }
    }
}

/// @brief Runs the example, printing each step.
void run() {
    World world;
    Components& components = world.getComponents();

    std::cout << "== Describing a component ==\n";
    // What a Luau file `export type Shield = { strength: f32, regen: f32 }` becomes.
    const ComponentInfo shieldInfo{
        .type = {.name = "example.Shield",
                 .size = sizeof(float) * 2,
                 .alignment = alignof(float),
                 .fields = {{.name = "strength", .type = FieldType::kF32, .offset = 0},
                            {.name = "regen", .type = FieldType::kF32, .offset = sizeof(float)}}}};
    const Result<ComponentId> registered = components.registerType(shieldInfo);
    const ComponentId shield = *registered;
    if (const std::optional<ComponentId> found = components.find("example.Shield")) {
        std::cout << std::format("registered 'example.Shield' as id {}; found by name: id {}\n", shield, *found);
    }

    std::cout << "\n== Adding a value as bytes ==\n";
    const Entity ship = world.spawn();
    std::vector<std::byte> value(sizeof(float) * 2);
    writeFloat(value, 0, 50.F);
    writeFloat(value, sizeof(float), 1.5F);
    components.add(ship, shield, value);
    printFields(components, ship, shield);

    std::cout << "\n== Writing a field ==\n";
    const std::span<std::byte> bytes = components.getMut(ship, shield);  // marked changed
    writeFloat(bytes, 0, readFloat(bytes, 0) + readFloat(bytes, sizeof(float)));
    printFields(components, ship, shield);

    std::cout << "\n== A wrong description is refused ==\n";
    // Two fields overlapping: registerType returns an Error instead of throwing (scripts must not crash the game).
    const ComponentInfo broken{.type = {.name = "example.Broken",
                                        .size = sizeof(float),
                                        .alignment = alignof(float),
                                        .fields = {{.name = "a", .type = FieldType::kF32, .offset = 0},
                                                   {.name = "b", .type = FieldType::kF32, .offset = 0}}}};
    const Result<ComponentId> refused = components.registerType(broken);
    if (!refused) {
        std::cout << std::format("refused: {}\n", refused.error().getMessage());
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
