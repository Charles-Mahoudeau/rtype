/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ResourceStorage
*/

#pragma once

#include <cstddef>
#include <memory_resource>
#include <span>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/change/ComponentTicks.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
#include "rtype/ecs/storage/ByteColumn.hpp"

namespace rtype::ecs::storage {
/// @brief The slot of one resource: no value, or a single value stored as raw bytes, with its change ticks.
///
/// @details The storage knows the resource only through its ResourceInfo, and keeps the value in a ByteColumn that
/// never holds more than one element: the column handles alignment and the lifecycle hooks. The buffer is allocated
/// at the first insertion and kept until the storage is destroyed, so moving a value in or removing it never
/// allocates. Replacing a value by copy builds the new one next to the old one first, so the buffer then holds two.
///
/// Spans returned by the storage are invalidated by insertMove(), insertCopy() and remove().
/// @code
/// const rtype::ecs::ResourceInfo info = rtype::ecs::ResourceTraits<Time>::makeInfo();
/// rtype::ecs::storage::ResourceStorage storage{info};
/// Time time{.delta = 0.F, .fixedDelta = 1.F / 60.F};
/// storage.insertMove(std::as_writable_bytes(std::span{&time, 1}), tick);
/// Time* stored = rtype::ecs::storage::viewAs<Time>(storage.get());
/// @endcode
class RTYPE_ECS_API ResourceStorage {
  public:
    /// @brief An empty slot for the described resource.
    /// @param info Must outlive the storage: the registry keeps every ResourceInfo at a stable address.
    /// @param memory Where the value's buffer comes from; it must outlive the storage.
    explicit ResourceStorage(const ResourceInfo& info,
                             std::pmr::memory_resource* memory = std::pmr::get_default_resource());
    /// @brief Destroys the value, if any.
    ~ResourceStorage() = default;
    ResourceStorage(const ResourceStorage& other) = delete;
    ResourceStorage& operator=(const ResourceStorage& other) = delete;
    ResourceStorage(ResourceStorage&& other) noexcept = default;
    ResourceStorage& operator=(ResourceStorage&& other) noexcept = default;

    /// @return The description of the stored resource.
    [[nodiscard]] const ResourceInfo& getInfo() const noexcept;

    /// @return When the value was inserted (a replacement keeps it) and last written; meaningless when empty.
    /// The mutable overload lets a Mut stamp `changed` on write access.
    [[nodiscard]] ComponentTicks& getTicks() noexcept;
    [[nodiscard]] const ComponentTicks& getTicks() const noexcept;

    /// @return Whether the slot holds a value.
    [[nodiscard]] bool hasValue() const noexcept;

    /// @return The bytes of the value; empty if there is none, and always for an empty type (use hasValue()).
    [[nodiscard]] std::span<std::byte> get() noexcept;
    [[nodiscard]] std::span<const std::byte> get() const noexcept;

    /// @brief Inserts a value moved from `source`, or replaces the current one (keeping its added tick).
    /// @details `source` stays a valid, moved-from value: its owner must still destroy it.
    /// @param source The bytes of a live value of this resource's type, exactly its size.
    /// @return The stored value's bytes, empty for an empty type.
    std::span<std::byte> insertMove(std::span<std::byte> source, Tick tick);

    /// @brief Inserts a copy of a value, or replaces the current one (keeping its added tick).
    /// @details If allocating or copying throws, the storage is unchanged: the old value is kept.
    /// @param source The bytes of a live value of this resource's type, exactly its size. The resource must be
    /// copyable (ResourceInfo::type.copyable).
    /// @return The stored value's bytes, empty for an empty type.
    std::span<std::byte> insertCopy(std::span<const std::byte> source, Tick tick);

    /// @brief Destroys the value. The buffer is kept for the next insertion.
    /// @return False, doing nothing, if the slot is empty.
    bool remove() noexcept;

    /// @brief Clamps both ticks so that, seen from `thisRun`, they are at most kMaxChangeAge old (see clampTick()).
    void clampTicks(Tick thisRun) noexcept;

  private:
    /// @brief Stamps the ticks of a value inserted at `tick`: a replacement keeps its added tick.
    void stampInsertion(bool replaced, Tick tick) noexcept;

    const ResourceInfo* _info;  ///< Description of the resource, owned by the registry.
    ByteColumn _value;          ///< The value, alone in the column once inserted.
    ComponentTicks _ticks;      ///< When the value was inserted and last written.
};
}  // namespace rtype::ecs::storage
