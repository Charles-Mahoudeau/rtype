/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Isolation
*/

#include "Isolation.hpp"

#include <glm/ext/vector_float2.hpp>
#include <optional>
#include <string>

#include "plugins/capsule/CapsuleSpawn.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/entity/Name.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/type/String.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::editor::scene {

std::optional<Isolation> isolate(ecs::World& world, plugins::capsule::CapsuleId id, std::string& error) {
    const auto* capsules = world.getResources().get<plugins::capsule::Capsules>();
    if (capsules == nullptr) {
        error = "The game has no capsules";
        return std::nullopt;
    }
    Isolation isolation{.capsule = id};
    try {
        ecs::Commands commands{world};
        isolation.anchor = commands.spawn()
                               .add(plugins::transform::Transform{.position = kIsolatedOrigin})
                               .add(ecs::Name{.value = ecs::String{"Prefab mode"}})
                               .getEntity();
        isolation.root = plugins::capsule::spawnCapsule(commands, *capsules, id)
                             .add(ecs::Parent{.entity = isolation.anchor})
                             .getEntity();
        commands.flush();
    } catch (const plugins::capsule::exceptions::CapsuleException& failure) {
        error = failure.what();
        return std::nullopt;
    }
    return isolation;
}

glm::vec2 centerOf(const ecs::World& world, const Isolation& isolation) {
    const auto* root = world.getComponents().get<plugins::transform::GlobalTransform>(isolation.root);
    return root != nullptr ? root->position : kIsolatedOrigin;
}

void endIsolation(ecs::World& world, const Isolation& isolation) {
    if (world.isAlive(isolation.anchor)) {
        static_cast<void>(world.despawn(isolation.anchor));
    }
}

}  // namespace rtype::editor::scene
