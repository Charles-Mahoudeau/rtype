/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PagedSparseArray
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

#include "rtype/ecs/Export.hpp"

namespace rtype::ecs::storage {
/// @brief Maps an entity index to a position in a pool's dense arrays, allocating memory only where indices are used.
///
/// @details The index space is cut into pages of kPageSize entries; a page is allocated the first time an index in
/// its range is set, and kept until the array is destroyed, so entities coming and going cause no allocation churn.
/// A component present on a few entities therefore costs a few pages, not an entry per possible entity.
///
/// Only the dense position is stored, not the generation: the pool rejects a stale entity by comparing the full
/// Entity kept in its dense array.
/// @code
/// rtype::ecs::storage::PagedSparseArray sparse;
/// sparse.set(entity.getIndex(), 3);
/// const std::optional<std::uint32_t> position = sparse.get(entity.getIndex());  // 3
/// sparse.erase(entity.getIndex());
/// @endcode
class RTYPE_ECS_API PagedSparseArray {
  public:
    /// @brief Entries per page (16 KB of 4-byte entries). A power of two, so splitting an index is a shift and a mask.
    static constexpr std::uint32_t kPageSize = 4096;

    PagedSparseArray() = default;
    ~PagedSparseArray() = default;
    PagedSparseArray(const PagedSparseArray& other) = delete;
    PagedSparseArray& operator=(const PagedSparseArray& other) = delete;
    PagedSparseArray(PagedSparseArray&& other) noexcept = default;
    PagedSparseArray& operator=(PagedSparseArray&& other) noexcept = default;

    /// @return The number of pages allocated so far.
    [[nodiscard]] std::size_t getAllocatedPageCount() const noexcept;

    /// @brief Maps an index to a dense position, allocating the index's page if needed.
    /// @param position Any value but the largest std::uint32_t, which means "no entry".
    void set(std::uint32_t index, std::uint32_t position);

    /// @return The dense position of the index, or nothing if it was never set or was erased.
    [[nodiscard]] std::optional<std::uint32_t> get(std::uint32_t index) const noexcept;

    /// @brief Removes the index's entry, if any. The page is kept.
    void erase(std::uint32_t index) noexcept;

    /// @brief Removes every entry. The pages are kept.
    void clear() noexcept;

  private:
    static_assert((kPageSize & (kPageSize - 1)) == 0, "the page size must be a power of two");

    using Page = std::array<std::uint32_t, kPageSize>;

    /// @brief Entry value meaning "no position".
    static constexpr std::uint32_t kEmpty = std::numeric_limits<std::uint32_t>::max();

    /// @return The allocated page holding the index, or nullptr.
    [[nodiscard]] Page* findPage(std::uint32_t index) noexcept;
    [[nodiscard]] const Page* findPage(std::uint32_t index) const noexcept;

    std::vector<std::unique_ptr<Page>> _pages;  ///< Pages by number (index / kPageSize); nullptr when not allocated.
    std::size_t _allocatedPageCount{0};         ///< Number of non-null pages.
};
}  // namespace rtype::ecs::storage
