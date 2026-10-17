/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Commands
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/storage/ByteColumn.hpp"

namespace rtype::ecs {
class World;

/// @brief A buffer of structural changes (spawn, add, remove, despawn) recorded now and applied later, in the order
/// they were recorded.
///
/// @details A system iterating a query must not change the pools it walks; it records the changes here, and they are
/// applied at the flush, between phases. spawn() hands out an entity at once (reserved, alive at the flush), so later
/// commands can use it. Values are moved into the buffer; a C++ component not registered yet is registered at the
/// flush, never while recording (registering can move every pool under a running query). Commands on an entity that
/// is dead when they are applied are skipped. A buffer destroyed or cleared before its flush destroys its values and
/// cancels the entities it reserved. It keeps a pointer to the world: the world must outlive it.
/// @code
/// rtype::ecs::Commands commands{world};
/// const rtype::ecs::Entity bullet = commands.spawn();
/// commands.add(bullet, Velocity{.x = 600.F});
/// commands.despawn(enemy);
/// commands.flush();  // the scheduler flushes between phases
/// @endcode
class RTYPE_ECS_API Commands {
  public:
    /// @param world The world the commands apply to; it must outlive the buffer.
    explicit Commands(World& world) noexcept;
    /// @brief Discards the commands not flushed (see clear()).
    ~Commands();
    Commands(const Commands& other) = delete;
    Commands& operator=(const Commands& other) = delete;
    /// @brief Takes the commands of `other`, which is left empty.
    Commands(Commands&& other) noexcept;
    /// @brief Discards this buffer's commands, then takes those of `other`, which is left empty.
    Commands& operator=(Commands&& other) noexcept;

    /// @return The number of commands recorded and not flushed yet.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @return Whether no command is waiting (a spawn with no component counts as none, but its entity is reserved).
    [[nodiscard]] bool isEmpty() const noexcept;

    /// @return A new entity, usable at once in other commands, alive from the next flush.
    /// @throws exceptions::EntityLimitException When every entity index is in use or retired.
    [[nodiscard]] Entity spawn();

    /// @brief Records giving the entity a C++ component, moved in, or replacing its value.
    /// @details `T` is registered at the flush if it is not registered yet.
    template <Component T>
    void add(Entity entity, T value);

    /// @brief Records giving the entity a copy of a value, or replacing its value (the Luau bridge, the network).
    /// @param value The bytes of a live value of the component, exactly its size (empty for a tag).
    /// @return False, recording nothing, if the id is unknown or `value` has the wrong size.
    bool add(Entity entity, ComponentId id, std::span<const std::byte> value);

    /// @brief Records removing a C++ component from the entity.
    template <Component T>
    void remove(Entity entity);

    /// @brief Records removing a component from the entity.
    /// @return False, recording nothing, if the id is unknown.
    bool remove(Entity entity, ComponentId id);

    /// @brief Records despawning the entity (World::despawn()).
    void despawn(Entity entity);

    /// @brief Applies every command in the order it was recorded, then empties the buffer.
    /// @details First registers the C++ components it needs (if that throws, nothing is applied and the buffer is
    /// kept), then makes the reserved entities alive, then applies each command; one on an entity dead by then is
    /// skipped. The buffer can be reused afterwards and keeps its capacity.
    /// @throws exceptions::ComponentRegistrationException When a component must be registered and its name is taken.
    void flush();

    /// @brief Discards every command not flushed: destroys the values and cancels the entities this buffer reserved
    /// (or despawns them, if another buffer's flush already made them alive).
    void clear() noexcept;

  private:
    /// @brief What a recorded command does.
    enum class Kind : std::uint8_t {
        kAdd,      ///< Give the entity the value at `index` in the column.
        kRemove,   ///< Remove the column's component from the entity.
        kDespawn,  ///< Despawn the entity.
    };

    /// @brief The values recorded for one component, and how to find its id at the flush.
    struct Column {
        std::uint64_t key;                    ///< The component's layout hash: one column per component.
        ComponentId id;                       ///< Its id, once known.
        ComponentId (*resolve)(Components&);  ///< Registers a C++ component at the flush; null when `id` is known.
        storage::ByteColumn values;           ///< The values recorded, moved in.
    };

    /// @brief One recorded command.
    struct Record {
        Kind kind;             ///< What it does.
        Entity entity;         ///< The entity it applies to.
        std::uint32_t column;  ///< Index in _columns of its component (add and remove).
        std::uint32_t index;   ///< Index of its value in the column (add).
    };

    /// @return The index of the column of the C++ component `T`, creating it (without registering `T`) if needed.
    template <Component T>
    [[nodiscard]] std::uint32_t columnOf();

    /// @return The index of the column of the registered component `id`, creating it if needed; nothing if the id
    /// is unknown.
    [[nodiscard]] std::optional<std::uint32_t> columnOf(ComponentId id);

    /// @return The index of the column with this key, or nothing.
    [[nodiscard]] std::optional<std::uint32_t> findColumn(std::uint64_t key) const noexcept;

    /// @return The id of `T` in `components`, registering it with the default options if needed.
    template <Component T>
    [[nodiscard]] static ComponentId registerIfNeeded(Components& components);

    World* _world;                  ///< The world the commands apply to.
    std::vector<Record> _records;   ///< The commands, in the order they were recorded.
    std::vector<Column> _columns;   ///< The values, one column per component used.
    std::vector<Entity> _reserved;  ///< Entities this buffer reserved and has not flushed.
};
}  // namespace rtype::ecs

#include "Commands.tpp"
