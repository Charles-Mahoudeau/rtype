/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** World
*/

#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/entity/EntityAllocator.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"

namespace rtype::ecs {
/// @brief The container of one simulation: its entities, its components and their values.
///
/// @details A world owns an entity allocator, a component registry and one pool per registered component. Nothing
/// is static, so several worlds coexist (a server and a client world, one world per game room, one per test);
/// component ids are per world, and only names are shared across worlds and processes.
/// @code
/// rtype::ecs::World world;
/// const rtype::ecs::Entity ship = world.spawn();
/// const rtype::ecs::ComponentId health = world.component<Health>();
/// @endcode
class RTYPE_ECS_API World {
  public:
    World() = default;
    /// @brief Destroys every component value.
    ~World() = default;
    World(const World& other) = delete;
    World& operator=(const World& other) = delete;
    World(World&& other) noexcept = default;
    World& operator=(World&& other) noexcept = default;

    /// @return The components registered in this world.
    [[nodiscard]] const ComponentRegistry& getRegistry() const noexcept;

    /// @return The world's change tick, stamped on every component added or replaced.
    [[nodiscard]] Tick getChangeTick() const noexcept;

    /// @return The number of entities alive.
    [[nodiscard]] std::size_t getEntityCount() const noexcept;

    /// @brief Creates an entity with no component.
    /// @throws exceptions::EntityLimitException When every entity index is in use or retired.
    [[nodiscard]] Entity spawn();

    /// @return Whether the entity was spawned in this world and not despawned since.
    [[nodiscard]] bool isAlive(Entity entity) const noexcept;

    /// @brief Registers a C++ component, or returns its id if it is already registered with the same options.
    /// @details Call it before the component is first added to set non-default options: adding registers the
    /// component with the default ones.
    /// @throws exceptions::ComponentRegistrationException When the name is taken by another layout or other
    /// options.
    template <Component T>
    ComponentId component(ComponentOptions options = {});

    /// @brief Registers a component described at run time (a Luau component), and creates its pool.
    /// @return Its id (the existing one if the same description was registered before), or why it was refused.
    [[nodiscard]] Result<ComponentId> registerComponent(ComponentInfo info);

    /// @return The id of the component with this name in this world, or nothing if it is not registered.
    [[nodiscard]] std::optional<ComponentId> findComponent(std::string_view name) const noexcept;

    /// @brief Gives the entity a copy of a value, or replaces its value if it already has the component.
    /// @details Used by the Luau bridge and the network, which only know components by id: failures they can cause
    /// are return values. Stamps the world's change tick (only as the changed tick when replacing).
    /// @param value The bytes of a live value of the component, exactly its size (empty for a tag).
    /// @return False, doing nothing, if the entity is not alive, the id is unknown, or `value` has the wrong size.
    bool add(Entity entity, ComponentId id, std::span<const std::byte> value);

    /// @return Whether the entity has the component; false for an unknown id.
    [[nodiscard]] bool has(Entity entity, ComponentId id) const noexcept;

    /// @return The bytes of the entity's value; empty if it does not have the component or the id is unknown, and
    /// always for a tag (use has() to test a tag). Invalidated by the next change to the component's pool.
    [[nodiscard]] std::span<const std::byte> get(Entity entity, ComponentId id) const noexcept;

    /// @brief Removes the component from the entity, destroying its value.
    /// @return False, doing nothing, if the entity does not have the component or the id is unknown.
    bool remove(Entity entity, ComponentId id) noexcept;

  private:
    /// @brief Creates the pools of every registered component up to `id` that has none yet.
    void createPools(ComponentId id);

    /// @return The pool of the component, or nullptr if the id is not registered in this world.
    [[nodiscard]] storage::SparseSetStorage* findPool(ComponentId id) noexcept;
    [[nodiscard]] const storage::SparseSetStorage* findPool(ComponentId id) const noexcept;

    EntityAllocator _entities;                      ///< Entity indices and generations.
    ComponentRegistry _registry;                    ///< Components of this world, by id and name.
    std::vector<storage::SparseSetStorage> _pools;  ///< One pool per component, indexed by ComponentId.
    Tick _changeTick{0};                            ///< Current tick; advanced by the scheduler later.
};
}  // namespace rtype::ecs

#include "World.tpp"
