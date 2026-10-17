/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Components
*/

#pragma once

#include <cstddef>
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
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"

namespace rtype::ecs {
namespace detail {
struct WorldState;
}  // namespace detail

/// @brief The components of a world: their registry, one pool per component, and the values in them.
///
/// @details Reached through World::getComponents(); it only exists inside a world, whose entities and change tick it
/// reads. Component ids are per world, and only names are shared across worlds and processes.
/// @code
/// rtype::ecs::Components& components = world.getComponents();
/// components.add(ship, Health{.value = 100});
/// const Health* health = components.get<Health>(ship);
/// @endcode
class RTYPE_ECS_API Components {
  public:
    /// @param state The state of the world these components belong to; it must outlive them.
    explicit Components(const detail::WorldState& state) noexcept;
    /// @brief Destroys every component value.
    ~Components() = default;
    Components(const Components& other) = delete;
    Components& operator=(const Components& other) = delete;
    Components(Components&& other) = delete;
    Components& operator=(Components&& other) = delete;

    /// @return The components registered in this world.
    [[nodiscard]] const ComponentRegistry& getRegistry() const noexcept;

    /// @brief Registers a C++ component, or returns its id if it is already registered with the same options.
    /// @details Call it before the component is first added to set non-default options: adding registers the
    /// component with the default ones.
    /// @throws exceptions::ComponentRegistrationException When the name is taken by another layout, other
    /// options, a resource or an event.
    template <Component T>
    ComponentId registerType(ComponentOptions options = {});

    /// @brief Registers a component described at run time (a Luau component), and creates its pool.
    /// @return Its id (the existing one if the same description was registered before), or why it was refused.
    [[nodiscard]] Result<ComponentId> registerType(ComponentInfo info);

    /// @return The id of the component with this name in this world, or nothing if it is not registered.
    [[nodiscard]] std::optional<ComponentId> find(std::string_view name) const noexcept;

    /// @brief Gives the entity a copy of a value, or replaces its value if it already has the component.
    /// @details Used by the Luau bridge and the network, which only know components by id: failures they can cause
    /// are return values. Stamps the world's change tick (only as the changed tick when replacing).
    /// @param value The bytes of a live value of the component, exactly its size (empty for a tag).
    /// @return False, doing nothing, if the entity is not alive, the id is unknown, or `value` has the wrong size.
    bool add(Entity entity, ComponentId id, std::span<const std::byte> value);

    /// @brief Gives the entity a value moved from `value`, or replaces its value if it already has the component.
    /// @details The type-erased move path: commands move the values they recorded into the pools at the flush. Stamps
    /// the world's change tick (only as the changed tick when replacing). `value` stays a valid, moved-from value: its
    /// owner must still destroy it.
    /// @param value The bytes of a live value of the component, exactly its size (empty for a tag).
    /// @return False, doing nothing (`value` is not moved from), if the entity is not alive, the id is unknown, or
    /// `value` has the wrong size.
    bool addMove(Entity entity, ComponentId id, std::span<std::byte> value);

    /// @return Whether the entity has the component; false for an unknown id.
    [[nodiscard]] bool has(Entity entity, ComponentId id) const noexcept;

    /// @return The bytes of the entity's value; empty if it does not have the component or the id is unknown, and
    /// always for a tag (use has() to test a tag). Invalidated by the next change to the component's pool.
    [[nodiscard]] std::span<const std::byte> get(Entity entity, ComponentId id) const noexcept;

    /// @brief Write access to the entity's value, marked changed now: the caller only has bytes, so it cannot tell
    /// a read from a write. Use get() to only read.
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

    /// @brief Removes the entity from every pool, destroying its values. Called by World::despawn().
    void removeAll(Entity entity) noexcept;

    /// @brief Sets to null every entity field, in every pool, that holds exactly `entity`, and marks those values
    /// changed. A stale handle of the same index (an older generation) is left alone. Called by World::despawn().
    void clearReferencesTo(Entity entity) noexcept;

    /// @brief Clamps every added and changed tick of every pool to at most kMaxChangeAge older than `now`. Called by
    /// World::checkChangeTicks().
    void clampTicks(Tick now) noexcept;

  private:
    /// @return Whether the entity is alive in the world.
    [[nodiscard]] bool isAlive(Entity entity) const noexcept;

    /// @return The world's change tick, stamped on every value added or written.
    [[nodiscard]] Tick getChangeTick() const noexcept;

    /// @brief Checks that `name` is not taken by another kind of type (WorldState::checkNameFree()). A member, so the
    /// templates, which only see a declaration of WorldState, can call it.
    /// @return Nothing when the name is free, or an ErrorKind::kNameCollision error.
    [[nodiscard]] Result<void> checkNameFree(std::string_view name) const;

    /// @brief Creates the pools of every registered component up to `id` that has none yet.
    void createPools(ComponentId id);

    /// @return The pool of the component, or nullptr if the id is not registered in this world.
    [[nodiscard]] storage::SparseSetStorage* findPool(ComponentId id) noexcept;
    [[nodiscard]] const storage::SparseSetStorage* findPool(ComponentId id) const noexcept;

    const detail::WorldState* _state;               ///< The world's state, read for its entities and change tick.
    ComponentRegistry _registry;                    ///< Components of this world, by id and name.
    std::vector<storage::SparseSetStorage> _pools;  ///< One pool per component, indexed by ComponentId.
};
}  // namespace rtype::ecs

#include "Components.tpp"
