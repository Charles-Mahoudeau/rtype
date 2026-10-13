/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ByteColumn
*/

#pragma once

#include <cstddef>
#include <memory>
#include <memory_resource>
#include <span>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"

namespace rtype::ecs::storage {
/// @brief A growable array of the values of one component type, stored as raw bytes.
///
/// @details The column knows a component only through its ComponentInfo: the size and alignment of a value, and
/// its lifecycle hooks. It constructs, moves and destroys values through those hooks, or with plain byte copies when
/// a hook is `nullptr` (plain data, and every Luau component). That is what lets pools store components that have
/// no C++ type.
///
/// Memory comes from a `std::pmr::memory_resource` (the default heap unless another one is given), allocated with
/// the component's alignment, so every value is correctly aligned. A tag (size 0) never allocates: the column only
/// counts its elements.
///
/// Values are exchanged as byte spans of exactly getElementSize() bytes (empty for a tag); viewAs() turns one into
/// the C++ type it holds. Spans returned by the column are invalidated by any insertion (the buffer may grow) and by
/// swapRemove() (the last value moves).
/// @code
/// rtype::ecs::storage::ByteColumn column{rtype::ecs::ComponentTraits<Health>::makeInfo()};
/// Health* health = rtype::ecs::storage::viewAs<Health>(column.emplaceBack());
/// column.emplaceBackCopy(std::as_bytes(std::span{&otherHealth, 1}));
/// column.swapRemove(0);
/// @endcode
class RTYPE_ECS_API ByteColumn {
  public:
    /// @brief An empty column for the described component.
    /// @param info Only its size, alignment and hooks are used; they are copied, so `info` may go away.
    /// @param resource Where the buffer comes from; it must outlive the column.
    explicit ByteColumn(const ComponentInfo& info,
                        std::pmr::memory_resource* resource = std::pmr::get_default_resource());
    /// @brief Destroys every value and gives the buffer back to its resource.
    ~ByteColumn();
    ByteColumn(const ByteColumn& other) = delete;
    ByteColumn& operator=(const ByteColumn& other) = delete;
    /// @brief Takes the values of `other`, which is left empty and usable.
    ByteColumn(ByteColumn&& other) noexcept;
    /// @brief Destroys this column's values, then takes those of `other`, which is left empty and usable.
    ByteColumn& operator=(ByteColumn&& other) noexcept;

    /// @return The number of values.
    [[nodiscard]] std::size_t getSize() const noexcept;

    /// @return The number of values the buffer holds before it has to grow.
    [[nodiscard]] std::size_t getCapacity() const noexcept;

    /// @return The size of one value in bytes; 0 for a tag.
    [[nodiscard]] std::size_t getElementSize() const noexcept;

    /// @brief Appends a default value: the construct hook, or zero-filled bytes when it is `nullptr`.
    /// @return The new value's bytes, empty for a tag.
    std::span<std::byte> emplaceBack();

    /// @brief Appends a copy of a value: the copy hook, or a byte copy when it is `nullptr`.
    /// @param source The bytes of a live value of this column's type, getElementSize() bytes long.
    /// @return The new value's bytes, empty for a tag.
    std::span<std::byte> emplaceBackCopy(std::span<const std::byte> source);

    /// @brief Appends a value moved from `source`: the move hook, or a byte copy when it is `nullptr`.
    /// @details `source` stays a valid, moved-from value: its owner must still destroy it.
    /// @param source The bytes of a live value of this column's type, getElementSize() bytes long.
    /// @return The new value's bytes, empty for a tag.
    std::span<std::byte> emplaceBackMove(std::span<std::byte> source);

    /// @brief Removes a value in constant time: the last value moves into its place.
    /// @param position Must be below getSize().
    void swapRemove(std::size_t position) noexcept;

    /// @return The bytes of the value at `position`, empty for a tag.
    /// @note Not checked in release builds: `position` must be below getSize().
    [[nodiscard]] std::span<std::byte> at(std::size_t position) noexcept;
    [[nodiscard]] std::span<const std::byte> at(std::size_t position) const noexcept;

    /// @brief Grows the buffer so it holds at least `capacity` values without growing again.
    void reserve(std::size_t capacity);

    /// @brief Destroys every value. The buffer is kept.
    void clear() noexcept;

  private:
    /// @brief Gives a buffer back to the resource it came from, with the size and alignment it was allocated with.
    struct BufferDelete {
        std::pmr::memory_resource* resource{nullptr};  ///< Resource the buffer came from.
        std::size_t bytes{0};                          ///< Size of the allocation.
        std::size_t alignment{1};                      ///< Alignment of the allocation.

        void operator()(std::byte* data) const noexcept;
    };

    using Buffer = std::unique_ptr<std::byte, BufferDelete>;

    /// @brief Makes room for one more value, doubling the capacity (at least 8) when the buffer is full.
    void makeRoom();

    /// @brief Moves every value to a new buffer of `capacity` values. The column is unchanged if allocation fails.
    void growTo(std::size_t capacity);

    /// @return The bytes of the value at `position` in a buffer of this column's capacity.
    [[nodiscard]] std::span<std::byte> slot(std::byte* data, std::size_t position) const noexcept;

    std::size_t _elementSize;              ///< Bytes per value; 0 for a tag.
    std::size_t _alignment;                ///< Alignment of every value.
    ComponentHooks _hooks;                 ///< Lifecycle of the values; `nullptr` hooks mean plain bytes.
    std::pmr::memory_resource* _resource;  ///< Where the buffer comes from.
    Buffer _data;                          ///< The values, `_elementSize` bytes each; null until the first growth.
    std::size_t _size{0};                  ///< Number of values.
    std::size_t _capacity{0};              ///< Number of values the buffer holds.
};
}  // namespace rtype::ecs::storage
