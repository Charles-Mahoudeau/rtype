/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CallContext
*/

#pragma once

#include <cstddef>
#include <span>

#include "Export.hpp"
#include "Value.hpp"
#include "ValuePack.hpp"

namespace rtype::luau {
/// @brief Context of a single call from Luau to a native Function, given to its handler.
///
/// Holds the arguments of the call, in order. Move-only: a context belongs to one call.
class RTYPE_LUAU_API CallContext {
  public:
    /// @brief Creates a context holding the given arguments.
    /// @param args The arguments of the call, in order.
    explicit CallContext(ValuePack args) noexcept;

    /// @brief Creates a context without arguments.
    CallContext() = default;

    /// @brief Destroys the context.
    ~CallContext() = default;

    /// @brief Copy is deleted: a context belongs to a single call.
    CallContext(const CallContext&) = delete;

    /// @brief Copy is deleted: a context belongs to a single call.
    CallContext& operator=(const CallContext&) = delete;

    /// @brief Takes over another context.
    CallContext(CallContext&&) noexcept = default;

    /// @brief Takes over another context.
    /// @return A reference to this context.
    CallContext& operator=(CallContext&&) noexcept = default;

    /// @brief Gets the arguments of the call.
    /// @return A view over the arguments, in order.
    [[nodiscard]] std::span<const Value> args() const noexcept;

    /// @brief Gets the number of arguments of the call.
    /// @return The number of arguments.
    [[nodiscard]] std::size_t argCount() const noexcept;

  private:
    ValuePack _args;  ///< The arguments of the call, in order.
};
}  // namespace rtype::luau
