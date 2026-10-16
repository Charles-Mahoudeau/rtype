/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** World
*/

#pragma once

#include <cstddef>
#include <memory>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/detail/WorldState.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/resource/Resources.hpp"

namespace rtype::ecs {
/// @brief The container of one simulation: its entities, its components, its resources and their values.
///
/// @details A world owns an entity allocator, its Components (a registry and one pool per registered component)
/// and its Resources (a registry and one slot per registered resource: a single value owned by the world, like the
/// time, the input or the renderer). The world itself does what involves several of them: spawning and despawning
/// entities, and moving the change tick that every part stamps. Nothing is static, so several worlds coexist (a server
/// and a client world, one world per game room, one per test); component and resource ids are per world, and only names
/// are shared across worlds and processes.
/// @code
/// rtype::ecs::World world;
/// const rtype::ecs::Entity ship = world.spawn();
/// world.getComponents().add(ship, Health{.value = 100});
/// world.getResources().insert(Time{});
/// @endcode
class RTYPE_ECS_API World {
  public:
    World() = default;
    /// @brief Destroys every component and resource value.
    ~World() = default;
    World(const World& other) = delete;
    World& operator=(const World& other) = delete;
    World(World&& other) noexcept = default;
    World& operator=(World&& other) noexcept = default;

    /// @return The components of this world: their registry, pools and values.
    [[nodiscard]] Components& getComponents() noexcept;
    [[nodiscard]] const Components& getComponents() const noexcept;

    /// @return The resources of this world: their registry and values.
    [[nodiscard]] Resources& getResources() noexcept;
    [[nodiscard]] const Resources& getResources() const noexcept;

    /// @return The world's change tick, stamped on every component added or replaced.
    [[nodiscard]] Tick getChangeTick() const noexcept;

    /// @return The number of entities alive.
    [[nodiscard]] std::size_t getEntityCount() const noexcept;

    /// @brief Moves the world's change tick forward; values added or written from now on get the new tick.
    /// @details Called once per system run by the scheduler. Every kCheckTickThreshold ticks it also clamps the
    /// ticks stored in the pools (checkChangeTicks()), so comparisons stay right when the 32-bit tick wraps around.
    /// @param count How many ticks to move forward (the scheduler runs several systems; tests jump far ahead).
    /// @return The new change tick.
    Tick advanceChangeTick(Tick count = 1) noexcept;

    /// @brief Clamps every added and changed tick of every pool and resource to at most kMaxChangeAge old.
    /// @details Runs automatically from advanceChangeTick(); calling it more often is harmless. Ticks kept outside
    /// the world (a system's last run, a client's acknowledged tick) must be clamped by their owner the same way.
    void checkChangeTicks() noexcept;

    /// @brief Creates an entity with no component.
    /// @throws exceptions::EntityLimitException When every entity index is in use or retired.
    [[nodiscard]] Entity spawn();

    /// @brief Creates an entity with the given C++ components, moved in as by add() (registering them if needed).
    /// @details If a type appears twice, the later value wins. If adding a component throws, the entity is
    /// despawned before the exception leaves: no half-built entity is left behind.
    /// @code
    /// const rtype::ecs::Entity ship = world.spawn(Position{.x = 10.F, .y = 20.F}, Velocity{}, Player{});
    /// @endcode
    /// @throws exceptions::EntityLimitException When every entity index is in use or retired.
    template <Component First, Component... Rest>
    Entity spawn(First first, Rest... rest);

    /// @brief Destroys an entity: removes it from every pool (destroying its values), clears the entity fields of
    /// other components that pointed to it, then frees its index.
    /// @details Every handle to it becomes stale, and its index can be reused under a new generation. A cleared field
    /// reads as the null entity and its value is marked changed. Clearing looks at every value of the components
    /// that have entity fields, so its cost grows with them, not with the whole world.
    /// @return False, doing nothing, if the entity is not alive.
    bool despawn(Entity entity) noexcept;

    /// @return Whether the entity was spawned in this world and not despawned since.
    [[nodiscard]] bool isAlive(Entity entity) const noexcept;

  private:
    /// @brief Everything the world owns, on the heap so that its address survives moves. A moved-from world has none:
    /// it can only be destroyed or assigned to.
    std::unique_ptr<detail::WorldState> _state{std::make_unique<detail::WorldState>()};
};
}  // namespace rtype::ecs

#include "World.tpp"
