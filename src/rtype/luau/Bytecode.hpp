/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Bytecode
*/

#pragma once

#include <cstddef>

#include "CHelper.hpp"
#include "Export.hpp"

namespace rtype::luau {
/// @brief Compiled Luau bytecode buffer.
///
/// Owns the buffer produced by the Luau compiler (allocated with `malloc`, released with `std::free`)
/// together with its size. Move-only: the buffer has a single owner.
class RTYPE_LUAU_API Bytecode {
  public:
    /// @brief Takes ownership of a bytecode buffer.
    /// @param data Buffer allocated by the Luau compiler.
    /// @param size Size of the buffer in bytes.
    // NOLINTNEXTLINE(*-avoid-c-arrays)
    explicit Bytecode(CPtr<char[]> data, std::size_t size);

    /// @brief Releases the bytecode buffer.
    ~Bytecode() = default;

    /// @brief Copying is disabled: the buffer has a single owner.
    Bytecode(const Bytecode&) noexcept = delete;

    /// @brief Copying is disabled: the buffer has a single owner.
    Bytecode& operator=(const Bytecode&) noexcept = delete;

    /// @brief Transfers ownership of the buffer from another instance.
    Bytecode(Bytecode&&) noexcept = default;

    /// @brief Transfers ownership of the buffer from another instance, releasing the current one.
    Bytecode& operator=(Bytecode&&) noexcept = default;

    /// @brief Gets the bytecode buffer.
    /// @return The owning pointer to the buffer; use `get()` to access the raw bytes.
    // NOLINTNEXTLINE(*-avoid-c-arrays)
    [[nodiscard]] const CPtr<char[]>& data() const noexcept;

    /// @brief Gets the size of the bytecode buffer.
    /// @return Size in bytes.
    [[nodiscard]] std::size_t size() const noexcept;

  private:
    // NOLINTNEXTLINE(*-avoid-c-arrays)
    CPtr<char[]> _data;  ///< Compiled bytecode buffer.
    std::size_t _size;   ///< Size of the buffer in bytes.
};
}  // namespace rtype::luau
