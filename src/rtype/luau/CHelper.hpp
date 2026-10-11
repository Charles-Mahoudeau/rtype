/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CHelper
*/

#pragma once

#include <cstdlib>
#include <memory>

namespace rtype::luau {
/// @brief Deleter releasing memory allocated by a C API with `malloc`.
///
/// Meant to be used as the deleter of a `std::unique_ptr` owning a buffer that a C library (e.g. Luau)
/// allocated and handed over, and that must be released with `std::free`.
struct FreeDeleter {
    /// @brief Frees the memory pointed to by @p ptr.
    /// @param ptr Pointer returned by `malloc` (or compatible); a null pointer is accepted and ignored.
    void operator()(void* ptr) const noexcept {
        // NOLINTNEXTLINE
        std::free(ptr);
    }
};

/// @brief Owning smart pointer for a C-allocated object, released with `std::free`.
/// @tparam T Type of the pointed-to object.
template <typename T>
using CPtr = std::unique_ptr<T, FreeDeleter>;
}  // namespace rtype::luau
