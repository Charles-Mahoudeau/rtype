/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** World
*/

#pragma once

#include <cstddef>
#include <deque>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/change/ComponentTicks.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/entity/EntityAllocator.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
#include "rtype/ecs/resource/ResourceRegistry.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/storage/ResourceStorage.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"

namespace rtype::ecs {
/// @brief The container of one simulation: its entities, its components, its resources and their values.
///
/// @details A world owns an entity allocator, a component registry and one pool per registered component, and a
/// resource registry and one slot per registered resource (a single value owned by the world: the time, the input,
/// the renderer). Nothing is static, so several worlds coexist (a server and a client world, one world per game
/// room, one per test); component and resource ids are per world, and only names are shared across worlds and
/// processes.
/// @code
/// rtype::ecs::World world;
/// const rtype::ecs::Entity ship = world.spawn();
/// const rtype::ecs::ComponentId health = world.component<Health>();
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

    /// @return The components registered in this world.
    [[nodiscard]] const ComponentRegistry& getRegistry() const noexcept;

    /// @return The resources registered in this world.
    [[nodiscard]] const ResourceRegistry& getResourceRegistry() const noexcept;

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

    /// @brief Write access to the entity's value, marked changed now: the caller only has bytes, so the World cannot
    /// tell a read from a write. Use get() to only read.
    /// @return The bytes of the value; empty if it does not have the component or the id is unknown, and always for
    /// a tag. Invalidated by the next change to the component's pool.
    [[nodiscard]] std::span<std::byte> getMut(Entity entity, ComponentId id) noexcept;

    /// @brief Removes the component from the entity, destroying its value.
    /// @return False, doing nothing, if the entity does not have the component or the id is unknown.
    bool remove(Entity entity, ComponentId id) noexcept;

    /// @return When the entity's value was added and last written, or nothing if it does not have the component or
    /// the id is unknown. Compare them with ComponentTicks::isAdded() / isChanged().
    [[nodiscard]] std::optional<ComponentTicks> getTicks(Entity entity, ComponentId id) const noexcept;

    /// @brief Gives the entity a C++ component, or replaces its value if it already has it.
    /// @details Registers `T` with the default options if it is not registered yet, then moves `value` in. Stamps
    /// the world's change tick (only as the changed tick when replacing).
    /// @return False, doing nothing (not even registering `T`), if the entity is not alive.
    /// @throws exceptions::ComponentRegistrationException When `T` is not registered and its name is already taken
    /// by another layout (a run-time component with the same name).
    template <Component T>
    bool add(Entity entity, T value);

    /// @return Whether the entity has the component; false if `T` is not registered in this world.
    template <Component T>
    [[nodiscard]] bool has(Entity entity) const noexcept;

    /// @return The entity's value, or nullptr if it does not have the component or `T` is not registered.
    /// Invalidated by the next change to the component's pool. Tags have no value: test them with has().
    template <Component T>
    [[nodiscard]] const T* get(Entity entity) const noexcept;

    /// @return Write access to the entity's value, which marks it changed only when written through (`->`, `*`);
    /// an empty Mut if it does not have the component or `T` is not registered. Invalidated by the next change to
    /// the component's pool. Tags have no value: test them with has().
    template <Component T>
    [[nodiscard]] Mut<T> getMut(Entity entity) noexcept;

    /// @brief Removes the C++ component from the entity, destroying its value.
    /// @return False, doing nothing, if the entity does not have the component or `T` is not registered.
    template <Component T>
    bool remove(Entity entity) noexcept;

    /// @return When the entity's value was added and last written, or nothing if it does not have the component or
    /// `T` is not registered.
    template <Component T>
    [[nodiscard]] std::optional<ComponentTicks> getTicks(Entity entity) const noexcept;

    /// @brief Registers a C++ resource, or returns its id if it is already registered with the same options.
    /// @details insertResource() and insertNonSendResource() register the resource themselves; call this to know
    /// its id, or to register it before its value exists.
    /// @throws exceptions::ResourceRegistrationException When the name is taken by another layout, or is registered
    /// with the other main-thread flag.
    template <Resource T>
    ResourceId resource(ResourceOptions options = {});

    /// @brief Registers a resource described at run time (a Luau resource), and creates its empty slot.
    /// @return Its id (the existing one if the same description was registered before), or why it was refused.
    [[nodiscard]] Result<ResourceId> registerResource(ResourceInfo info);

    /// @return The id of the resource with this name in this world, or nothing if it is not registered.
    [[nodiscard]] std::optional<ResourceId> findResource(std::string_view name) const noexcept;

    /// @brief Sets the resource to a copy of a value, replacing the current one if any.
    /// @details Used by the Luau bridge and the network, which only know resources by id. Stamps the world's change
    /// tick (only as the changed tick when replacing). If copying throws, the current value is kept.
    /// @param value The bytes of a live value of the resource, exactly its size (empty for an empty type).
    /// @return False, doing nothing, if the id is unknown, `value` has the wrong size, or the resource is not
    /// copyable.
    bool insertResource(ResourceId id, std::span<const std::byte> value);

    /// @return Whether the resource holds a value; false for an unknown id.
    [[nodiscard]] bool hasResource(ResourceId id) const noexcept;

    /// @return The bytes of the resource's value; empty if it has none or the id is unknown, and always for an empty
    /// type (use hasResource()). Invalidated by the next insertion or removal of the resource.
    [[nodiscard]] std::span<const std::byte> getResource(ResourceId id) const noexcept;

    /// @brief Write access to the resource's value, marked changed now: the caller only has bytes, so the World
    /// cannot tell a read from a write. Use getResource() to only read.
    /// @return The bytes of the value; empty if it has none or the id is unknown, and always for an empty type.
    [[nodiscard]] std::span<std::byte> getResourceMut(ResourceId id) noexcept;

    /// @brief Destroys the resource's value. The resource stays registered.
    /// @return False, doing nothing, if it has no value or the id is unknown.
    bool removeResource(ResourceId id) noexcept;

    /// @return When the resource's value was inserted and last written, or nothing if it has no value or the id is
    /// unknown.
    [[nodiscard]] std::optional<ComponentTicks> getResourceTicks(ResourceId id) const noexcept;

    /// @brief Sets a C++ resource, replacing its value if it already has one.
    /// @details Registers `T` (usable from any thread) if it is not registered yet, then moves `value` in. Stamps
    /// the world's change tick (only as the changed tick when replacing).
    /// @code
    /// world.insertResource(Time{.delta = 0.F, .fixedDelta = 1.F / 60.F});
    /// @endcode
    /// @throws exceptions::ResourceRegistrationException When `T` is registered as main-thread only, or its name is
    /// already taken by another layout (a run-time resource with the same name).
    template <Resource T>
    void insertResource(T value);

    /// @brief Sets a main-thread-only C++ resource (the platform, the renderer), replacing its value if any.
    /// @details Like insertResource(), but registers `T` as main-thread only: systems using it must run on the main
    /// thread.
    /// @throws exceptions::ResourceRegistrationException When `T` is registered as usable from any thread, or its
    /// name is already taken by another layout.
    template <Resource T>
    void insertNonSendResource(T value);

    /// @return Whether the C++ resource holds a value; false if `T` is not registered in this world.
    template <Resource T>
    [[nodiscard]] bool hasResource() const noexcept;

    /// @return The resource's value, or nullptr if it has none or `T` is not registered. Invalidated by the next
    /// insertion or removal of the resource. Empty types have no value: test them with hasResource().
    template <Resource T>
    [[nodiscard]] const T* getResource() const noexcept;

    /// @return Write access to the resource's value, which marks it changed only when written through (`->`, `*`);
    /// an empty Mut if it has no value or `T` is not registered. Invalidated by the next insertion or removal of the
    /// resource. Empty types have no value: test them with hasResource().
    template <Resource T>
    [[nodiscard]] Mut<T> getResourceMut() noexcept;

    /// @return The resource's value. Invalidated by the next insertion or removal of the resource.
    /// @throws exceptions::MissingResourceException When it has no value or `T` is not registered.
    template <Resource T>
    [[nodiscard]] const T& requireResource() const;

    /// @return Write access to the resource's value, which marks it changed only when written through (`->`, `*`).
    /// Invalidated by the next insertion or removal of the resource.
    /// @throws exceptions::MissingResourceException When it has no value or `T` is not registered.
    template <Resource T>
    [[nodiscard]] Mut<T> requireResourceMut();

    /// @brief Destroys the C++ resource's value. The resource stays registered.
    /// @return False, doing nothing, if it has no value or `T` is not registered.
    template <Resource T>
    bool removeResource() noexcept;

    /// @return When the resource's value was inserted and last written, or nothing if it has no value or `T` is not
    /// registered.
    template <Resource T>
    [[nodiscard]] std::optional<ComponentTicks> getResourceTicks() const noexcept;

  private:
    /// @brief Creates the pools of every registered component up to `id` that has none yet.
    void createPools(ComponentId id);

    /// @brief Sets to null every entity field, in every pool, that holds exactly `entity`, and marks those values
    /// changed. A stale handle of the same index (an older generation) is left alone.
    void clearReferencesTo(Entity entity) noexcept;

    /// @return The pool of the component, or nullptr if the id is not registered in this world.
    [[nodiscard]] storage::SparseSetStorage* findPool(ComponentId id) noexcept;
    [[nodiscard]] const storage::SparseSetStorage* findPool(ComponentId id) const noexcept;

    /// @brief Moves `value` into the slot of `T`, registering `T` with `options` if needed.
    template <Resource T>
    void insertResourceWith(T value, ResourceOptions options);

    /// @brief Creates the slots of every registered resource up to `id` that has none yet.
    void createResourceStorages(ResourceId id);

    /// @return The slot of the resource, or nullptr if the id is not registered in this world.
    [[nodiscard]] storage::ResourceStorage* findResourceStorage(ResourceId id) noexcept;
    [[nodiscard]] const storage::ResourceStorage* findResourceStorage(ResourceId id) const noexcept;

    EntityAllocator _entities;                      ///< Entity indices and generations.
    ComponentRegistry _registry;                    ///< Components of this world, by id and name.
    std::vector<storage::SparseSetStorage> _pools;  ///< One pool per component, indexed by ComponentId.
    ResourceRegistry _resourceRegistry;             ///< Resources of this world, by id and name.
    /// @brief One slot per resource, indexed by ResourceId; a deque keeps their addresses, so a Mut stays valid
    /// when other resources are registered.
    std::deque<storage::ResourceStorage> _resources;
    Tick _changeTick{0};     ///< Current tick, stamped on every value added or written.
    Tick _lastCheckTick{0};  ///< Change tick at the last checkChangeTicks().
};
}  // namespace rtype::ecs

#include "World.tpp"
