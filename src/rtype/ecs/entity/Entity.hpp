/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Entity
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>

namespace rtype::ecs {
/// @brief Generational handle to an entity: an index into the world's slots and the generation of that slot.
///
/// @details An entity has no data of its own: everything about it is the components attached to it. The
/// generation is bumped each time the slot is released, so a handle kept after its entity was released is
/// detected as stale instead of reaching the entity that reuses the slot. Trivially copyable and 8 bytes. A
/// default-constructed Entity (all bits zero) is the null entity; live entities always have a generation of at
/// least 1, so a zero-filled component field of type Entity reads as null.
///
/// Entities are local to one world: never send them over the network.
class Entity {
  public:
    /// @brief An entity with the given slot index and generation.
    constexpr Entity(std::uint32_t index, std::uint32_t generation) noexcept
        : _bits{(static_cast<std::uint64_t>(generation) << kGenerationShift) | index} {}
    /// @brief The null entity.
    constexpr Entity() noexcept = default;
    ~Entity() = default;
    constexpr Entity(const Entity& other) noexcept = default;
    constexpr Entity& operator=(const Entity& other) noexcept = default;
    constexpr Entity(Entity&& other) noexcept = default;
    constexpr Entity& operator=(Entity&& other) noexcept = default;

    /// @return The slot index in the world.
    [[nodiscard]] constexpr std::uint32_t getIndex() const noexcept { return static_cast<std::uint32_t>(_bits); }

    /// @return The generation of the slot when this handle was created; 0 for the null entity.
    [[nodiscard]] constexpr std::uint32_t getGeneration() const noexcept {
        return static_cast<std::uint32_t>(_bits >> kGenerationShift);
    }

    /// @return Index and generation packed in one value (generation in the high 32 bits), e.g. for hashing.
    [[nodiscard]] constexpr std::uint64_t getBits() const noexcept { return _bits; }

    /// @return True for the null entity.
    [[nodiscard]] constexpr bool isNull() const noexcept { return _bits == 0; }

    friend constexpr bool operator==(Entity lhs, Entity rhs) noexcept = default;

  private:
    static constexpr std::uint32_t kGenerationShift = 32;  ///< Position of the generation in _bits.

    std::uint64_t _bits{0};  ///< generation << 32 | index; 0 for the null entity.
};

static_assert(sizeof(Entity) == sizeof(std::uint64_t), "an Entity must stay 8 bytes");
static_assert(std::is_trivially_copyable_v<Entity>, "an Entity must be trivially copyable");
}  // namespace rtype::ecs

/// @brief Hashes an Entity, so it can be a key of unordered containers.
template <>
struct std::hash<rtype::ecs::Entity> {
    [[nodiscard]] std::size_t operator()(rtype::ecs::Entity entity) const noexcept {
        return std::hash<std::uint64_t>{}(entity.getBits());
    }
};
