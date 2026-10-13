/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SparseSetStorage
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <optional>
#include <span>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/storage/ByteColumn.hpp"
#include "rtype/ecs/storage/PagedSparseArray.hpp"

namespace rtype::ecs::storage {
/// @brief The pool of one component: every value of that component, packed, with the entity it belongs to.
///
/// @details A sparse array maps an entity index to a dense position; the dense arrays (entities, values, added
/// and changed ticks) are kept in the same order, so systems walk them linearly. Values are raw bytes described
/// by the component's ComponentInfo, so the pool also stores components that have no C++ type. A tag (size 0)
/// stores no bytes: its pool is only the entities and their ticks.
///
/// An entity is found only if the full handle matches, generation included: a stale handle whose index was
/// reused is not found.
///
/// Spans returned by the pool are invalidated by any insertion (the values may move to a larger buffer).
/// @code
/// rtype::ecs::storage::SparseSetStorage pool{info};
/// Health* health = rtype::ecs::storage::viewAs<Health>(pool.emplace(entity, tick));
/// const bool has = pool.contains(entity);
/// @endcode
class RTYPE_ECS_API SparseSetStorage {
  public:
    /// @brief An empty pool for the described component.
    /// @param info Must outlive the pool: the registry keeps every ComponentInfo at a stable address.
    /// @param resource Where the values' buffer comes from; it must outlive the pool.
    explicit SparseSetStorage(const ComponentInfo& info,
                              std::pmr::memory_resource* resource = std::pmr::get_default_resource());
    /// @brief Destroys every value.
    ~SparseSetStorage() = default;
    SparseSetStorage(const SparseSetStorage& other) = delete;
    SparseSetStorage& operator=(const SparseSetStorage& other) = delete;
    SparseSetStorage(SparseSetStorage&& other) noexcept = default;
    SparseSetStorage& operator=(SparseSetStorage&& other) noexcept = default;

    /// @return The description of the stored component.
    [[nodiscard]] const ComponentInfo& getInfo() const noexcept;

    /// @return The number of entities that have the component.
    [[nodiscard]] std::size_t getSize() const noexcept;

    /// @return The entities that have the component, in dense order.
    [[nodiscard]] std::span<const Entity> getEntities() const noexcept;

    /// @return The tick each value was added at, in dense order.
    [[nodiscard]] std::span<Tick> getAddedTicks() noexcept;
    [[nodiscard]] std::span<const Tick> getAddedTicks() const noexcept;

    /// @return The tick each value was last written at, in dense order.
    [[nodiscard]] std::span<Tick> getChangedTicks() noexcept;
    [[nodiscard]] std::span<const Tick> getChangedTicks() const noexcept;

    /// @return Whether the entity has the component.
    [[nodiscard]] bool contains(Entity entity) const noexcept;

    /// @return The entity's position in the dense arrays, or nothing if it does not have the component.
    [[nodiscard]] std::optional<std::uint32_t> find(Entity entity) const noexcept;

    /// @return The bytes of the entity's value; empty if it does not have the component, and always for a tag
    /// (use contains() to test a tag).
    [[nodiscard]] std::span<std::byte> get(Entity entity) noexcept;
    [[nodiscard]] std::span<const std::byte> get(Entity entity) const noexcept;

    /// @return The bytes of the value at a dense position, empty for a tag.
    /// @note Not checked in release builds: `position` must be below getSize().
    [[nodiscard]] std::span<std::byte> at(std::size_t position) noexcept;
    [[nodiscard]] std::span<const std::byte> at(std::size_t position) const noexcept;

    /// @brief Gives the entity a default value: the construct hook, or zero-filled bytes.
    /// @details If anything throws, the pool is unchanged.
    /// @param entity Must not be null, and its index must not be in the pool yet, under any generation: the
    /// caller decides what to do with an entity that already has the component.
    /// @param tick Stamped as both the added and the changed tick.
    /// @return The new value's bytes, empty for a tag.
    std::span<std::byte> emplace(Entity entity, Tick tick);

    /// @brief Grows every dense array so the pool holds at least `capacity` values without growing again.
    void reserve(std::size_t capacity);

  private:
    /// @brief Records the value just appended to the column for `entity`: dense entity, ticks and sparse entry.
    /// @details If that throws, the appended value is removed again, so the pool is unchanged.
    void track(Entity entity, Tick tick);

    const ComponentInfo* _info;  ///< The stored component, owned by the registry.
    PagedSparseArray _sparse;    ///< Entity index -> dense position.
    std::vector<Entity> _dense;  ///< Entities, packed.
    ByteColumn _data;            ///< Values, in the order of `_dense`; only counted for a tag.
    std::vector<Tick> _added;    ///< When each value was added.
    std::vector<Tick> _changed;  ///< When each value was last written.
};
}  // namespace rtype::ecs::storage
