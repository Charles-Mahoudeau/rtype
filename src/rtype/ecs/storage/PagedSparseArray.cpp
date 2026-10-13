/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PagedSparseArray
*/

#include "PagedSparseArray.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>

namespace rtype::ecs::storage {
namespace {
/// @return The entry of the index inside its page. The offset is always below kPageSize, so no check is needed.
template <typename PageType>
[[nodiscard]] auto& entryOf(PageType& page, std::uint32_t index) noexcept {
    return *std::next(page.begin(), static_cast<std::ptrdiff_t>(index % PagedSparseArray::kPageSize));
}
}  // namespace

std::size_t PagedSparseArray::getAllocatedPageCount() const noexcept { return _allocatedPageCount; }

void PagedSparseArray::set(std::uint32_t index, std::uint32_t position) {
    assert(position != kEmpty);  // kEmpty is reserved: it would read back as "no entry"
    const std::size_t pageNumber = index / kPageSize;
    if (pageNumber >= _pages.size()) {
        _pages.resize(pageNumber + 1);
    }
    std::unique_ptr<Page>& page = _pages.at(pageNumber);
    if (!page) {
        page = std::make_unique<Page>();
        page->fill(kEmpty);
        ++_allocatedPageCount;
    }
    entryOf(*page, index) = position;
}

std::optional<std::uint32_t> PagedSparseArray::get(std::uint32_t index) const noexcept {
    const Page* page = findPage(index);
    if (page == nullptr) {
        return std::nullopt;
    }
    const std::uint32_t entry = entryOf(*page, index);
    if (entry == kEmpty) {
        return std::nullopt;
    }
    return entry;
}

void PagedSparseArray::erase(std::uint32_t index) noexcept {
    if (Page* page = findPage(index); page != nullptr) {
        entryOf(*page, index) = kEmpty;
    }
}

void PagedSparseArray::clear() noexcept {
    for (const std::unique_ptr<Page>& page : _pages) {
        if (page) {
            page->fill(kEmpty);
        }
    }
}

PagedSparseArray::Page* PagedSparseArray::findPage(std::uint32_t index) noexcept {
    const std::size_t pageNumber = index / kPageSize;
    if (pageNumber >= _pages.size()) {
        return nullptr;
    }
    // In range: checked above, so the page is reached through an iterator rather than a throwing at().
    return std::next(_pages.begin(), static_cast<std::ptrdiff_t>(pageNumber))->get();
}

const PagedSparseArray::Page* PagedSparseArray::findPage(std::uint32_t index) const noexcept {
    const std::size_t pageNumber = index / kPageSize;
    if (pageNumber >= _pages.size()) {
        return nullptr;
    }
    return std::next(_pages.begin(), static_cast<std::ptrdiff_t>(pageNumber))->get();
}
}  // namespace rtype::ecs::storage
