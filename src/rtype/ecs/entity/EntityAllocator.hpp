/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EntityAllocator
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Entity.hpp"
#include "rtype/ecs/Export.hpp"

namespace rtype::ecs {
/// @brief Creates, recycles and validates entities.
///
/// @details Keeps one generation per slot and a free list of released slots, reused last-in first-out so recently
/// used memory stays warm. Releasing an entity bumps its slot's generation, which makes every handle to it stale.
/// A slot whose generation would overflow is retired and never reused.
///
/// `reserve()` hands out an entity that only becomes alive at the next `flushReserved()`: commands use it to give
/// a spawned entity an id before the world creates it.
///
/// @code
/// rtype::ecs::EntityAllocator allocator;
/// const rtype::ecs::Entity ship = allocator.allocate();
/// allocator.release(ship);
/// allocator.isAlive(ship);  // false: its slot's generation was bumped
/// @endcode
///
/// @note Single-threaded. `reserve()` will need to become atomic for a parallel scheduler.
class RTYPE_ECS_API EntityAllocator {
  public:
    EntityAllocator() = default;
    ~EntityAllocator() = default;
    EntityAllocator(const EntityAllocator& other) = default;
    EntityAllocator& operator=(const EntityAllocator& other) = default;
    EntityAllocator(EntityAllocator&& other) noexcept = default;
    EntityAllocator& operator=(EntityAllocator&& other) noexcept = default;

    /// @return The number of alive entities (reserved ones are not counted until flushed).
    [[nodiscard]] std::size_t getAliveCount() const noexcept;

    /// @brief Creates an entity, alive at once.
    /// @throws exceptions::EntityLimitException If every entity index is in use or retired.
    [[nodiscard]] Entity allocate();

    /// @brief Creates an entity that becomes alive at the next flushReserved().
    /// @throws exceptions::EntityLimitException If every entity index is in use or retired.
    [[nodiscard]] Entity reserve();

    /// @brief Makes every entity reserved since the last flush alive, except the ones released in between.
    void flushReserved();

    /// @brief Releases an alive entity, or cancels a reserved one. Its handle, and every copy of it, becomes stale.
    /// @return False, doing nothing, if the entity is not alive or reserved: null, unknown, stale or already
    /// released.
    bool release(Entity entity) noexcept;

    /// @return True if the entity exists and was not released since.
    [[nodiscard]] bool isAlive(Entity entity) const noexcept;

    /// @return True if the entity was reserved and is not yet flushed nor released.
    [[nodiscard]] bool isReserved(Entity entity) const noexcept;

  private:
    /// @brief What a slot currently holds.
    enum class SlotState : std::uint8_t {
        kFree,      ///< In the free list, waiting to be reused.
        kReserved,  ///< Handed out by reserve(), alive after the next flush.
        kAlive,     ///< Holds a live entity.
        kRetired,   ///< Its generation is at the maximum: never reused.
    };

    /// @brief One entry per index ever handed out.
    struct Slot {
        std::uint32_t generation;  ///< Generation of the current or next entity in this slot.
        SlotState state;           ///< What the slot currently holds.
    };

    /// @brief Takes a slot from the free list (last released first), or appends a new one, and puts it in a state.
    /// @throws exceptions::EntityLimitException If no index is left.
    [[nodiscard]] Entity take(SlotState state);

    /// @return The slot of a handle that matches it (same generation), or nullptr.
    [[nodiscard]] Slot* find(Entity entity) noexcept;
    [[nodiscard]] const Slot* find(Entity entity) const noexcept;

    std::vector<Slot> _slots;              ///< One per index ever handed out.
    std::vector<std::uint32_t> _freeList;  ///< Released indices, reused last-in first-out.
    std::vector<std::uint32_t> _reserved;  ///< Indices reserved since the last flush.
    std::size_t _aliveCount{0};            ///< Entities currently alive.
};
}  // namespace rtype::ecs
