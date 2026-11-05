/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldPoses
*/

#pragma once

#include <unordered_map>

#include "Transform.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"

namespace rtype::plugins::transform {
/// @brief Computes where entities are in the world from their Transforms and Parents, for systems that cannot wait for
/// GlobalTransform: the physics, in FixedUpdate. A Query cannot read one given entity, so it is filled from one.
/// @code
/// rtype::plugins::transform::WorldPoses poses;
/// for (auto [entity, local, parent] : transforms) {
///     poses.add(entity, local, parent);
/// }
/// const rtype::plugins::transform::GlobalTransform turret = poses.of(entity);
/// @endcode
class WorldPoses {
  public:
    WorldPoses() = default;
    ~WorldPoses() = default;
    WorldPoses(const WorldPoses& other) = delete;
    WorldPoses& operator=(const WorldPoses& other) = delete;
    WorldPoses(WorldPoses&& other) noexcept = default;
    WorldPoses& operator=(WorldPoses&& other) noexcept = default;

    /// @brief Adds an entity. `local` and `parent` must outlive this object.
    /// @param parent Its Parent, or null for a root.
    void add(ecs::Entity entity, const Transform& local, const ecs::Parent* parent);

    /// @return Where the entity is in the world, as the TransformPlugin would compute it: a parent that was not added
    /// ends the chain, as a parent without a Transform does. The default GlobalTransform for an entity not added.
    [[nodiscard]] GlobalTransform of(ecs::Entity entity) const;

  private:
    /// @brief What one entity gave to add().
    struct Entry {
        const Transform* local;     ///< Not owned.
        const ecs::Parent* parent;  ///< Not owned; null for a root.
    };

    std::unordered_map<ecs::Entity, Entry> _entries;  ///< By entity.
};
}  // namespace rtype::plugins::transform
