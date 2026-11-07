/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleSpawner
*/

#pragma once

#ifndef RTYPE_CAPSULE_SPAWNER_INCLUDING_TPP
#error "CapsuleSpawner.tpp is an implementation detail of CapsuleSpawner.hpp: include CapsuleSpawner.hpp instead"
#endif

#include <cstddef>
#include <iterator>
#include <vector>

#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/ComponentValues.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::plugins::capsule::detail {
template <CapsuleTarget Target>
void addKeptValues(const Capsules& capsules, const ComponentValues& values, ecs::Entity entity, Target& target) {
    for (std::size_t index = 0; index < values.getCount(); ++index) {
        if (capsules.keeps(values.getPresence(index))) {
            target.addValue(entity, values.getId(index), values.getValue(index));
        }
    }
}

template <CapsuleTarget Target>
// NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the instances, which checkCapsule() keeps acyclic.
ecs::Entity spawnTree(const Capsules& capsules, const Capsule& capsule, Target& target, ecs::Entity root, bool nested) {
    const auto nodes = capsule.getNodes();
    std::vector<ecs::Entity> entities(nodes.size());  // the null entity for a node skipped
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        const CapsuleNode& node = *std::next(nodes.begin(), static_cast<std::ptrdiff_t>(index));
        ecs::Entity parent;
        if (index != 0) {
            parent = entities.at(node.parent);
            if (parent == ecs::Entity{} || !capsules.loads(node.side)) {
                continue;  // its parent was skipped, or it is the other side's: it is skipped with its subtree
            }
        }
        const ecs::Entity into = index == 0 ? root : ecs::Entity{};
        ecs::Entity entity;
        if (node.instance) {
            entity = spawnTree(capsules, *capsules.find(node.instance->id), target, into, true);
        } else {
            entity = into != ecs::Entity{} ? into : target.spawnEntity();
        }
        addKeptValues(capsules, node.values, entity, target);
        if (index != 0) {
            target.setParent(entity, parent);
        }
        entities.at(index) = entity;
    }
    const ecs::Entity spawned = entities.front();
    target.addInstance(spawned,
                       CapsuleInstance{.capsule = capsule.getId(), .version = capsule.getVersion(), .nested = nested});
    return spawned;
}
}  // namespace rtype::plugins::capsule::detail
