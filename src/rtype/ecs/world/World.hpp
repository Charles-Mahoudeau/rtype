/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** World
*/

#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/change/ComponentTicks.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/detail/WorldState.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
#include "rtype/ecs/resource/ResourceRegistry.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/storage/ResourceStorage.hpp"

namespace rtype::ecs {
/// @brief The container of one simulation: its entities, its components, its resources and their values.
///
/// @details A world owns an entity allocator, its Components (a registry and one pool per registered component),
/// and a resource registry and one slot per registered resource (a single value owned by the world: the time, the
/// input, the renderer). Nothing is static, so several worlds coexist (a server and a client world, one world per game
/// room, one per test); component and resource ids are per world, and only names are shared across worlds and
/// processes.
/// @code
/// rtype::ecs::World world;
/// const rtype::ecs::Entity ship = world.spawn();
/// world.getComponents().add(ship, Health{.value = 100});
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

    /// @brief Registers a C++ resource, or returns its id if it is already registered with the same options.
    /// @details insertResource() and insertNonSendResource() register the resource themselves; call this to know
    /// its id, or to register it before its value exists.
    /// @throws exceptions::ResourceRegistrationException When the name is taken by another layout, by a component,
    /// or is registered with the other main-thread flag.
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
    /// @brief Checks that `name` is not a component: a name is either a component or a resource, so the Luau
    /// bridge, the network manifest and save files can resolve it alone.
    /// @return Nothing when the name is free, or an ErrorKind::kNameCollision error.
    [[nodiscard]] Result<void> checkNameFree(std::string_view name) const;

    /// @brief Moves `value` into the slot of `T`, registering `T` with `options` if needed.
    template <Resource T>
    void insertResourceWith(T value, ResourceOptions options);

    /// @brief Creates the slots of every registered resource up to `id` that has none yet.
    void createResourceStorages(ResourceId id);

    /// @return The slot of the resource, or nullptr if the id is not registered in this world.
    [[nodiscard]] storage::ResourceStorage* findResourceStorage(ResourceId id) noexcept;
    [[nodiscard]] const storage::ResourceStorage* findResourceStorage(ResourceId id) const noexcept;

    /// @brief Everything the world owns, on the heap so that its address survives moves. A moved-from world has none:
    /// it can only be destroyed or assigned to.
    std::unique_ptr<detail::WorldState> _state{std::make_unique<detail::WorldState>()};
};
}  // namespace rtype::ecs

#include "World.tpp"
