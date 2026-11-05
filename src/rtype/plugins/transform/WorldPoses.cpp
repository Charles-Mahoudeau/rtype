/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldPoses
*/

#include "WorldPoses.hpp"

#include <ranges>
#include <vector>

#include "Transform.hpp"
#include "TransformMath.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"

namespace rtype::plugins::transform {

void WorldPoses::add(ecs::Entity entity, const Transform& local, const ecs::Parent* parent) {
    _entries.insert_or_assign(entity, Entry{.local = &local, .parent = parent});
}

GlobalTransform WorldPoses::of(ecs::Entity entity) const {
    std::vector<const Transform*> chain;
    auto found = _entries.find(entity);
    while (found != _entries.end()) {
        chain.push_back(found->second.local);
        found = found->second.parent != nullptr ? _entries.find(found->second.parent->entity) : _entries.end();
    }
    GlobalTransform pose;
    for (const Transform* local : std::views::reverse(chain)) {
        pose = combine(pose, *local);
    }
    return pose;
}
}  // namespace rtype::plugins::transform
