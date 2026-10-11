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
#include "Result.hpp"
#include "Value.hpp"
#include "ValuePack.hpp"

struct lua_State;

namespace rtype::luau {
/// @brief Context of a single call from Luau to a native Function, given to its handler.
///
/// Built by the native closure from the arguments Luau passed to it (see `fromStack`), and holds them in order.
/// Move-only: a context belongs to one call.
class RTYPE_LUAU_API CallContext {
  public:
    /// @brief Reads the arguments of a call to a native closure from the stack of a Lua state, without popping them.
    ///
    /// Meant to be called from a C function: every value of the stack, from index 1 up, is an argument.
    /// @param state The Lua state calling the closure; must not be null.
    /// @return A context holding a copy of the arguments in order, or a Failure of kind `kTypeMismatch` if one of
    ///         them has a type that cannot be read yet (see `ValuePack::fromStack`).
    [[nodiscard]] static Result<CallContext> fromStack(lua_State* state);

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
