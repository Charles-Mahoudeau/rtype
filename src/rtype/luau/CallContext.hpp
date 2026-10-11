/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CallContext
*/

#pragma once

#include <cstddef>
#include <vector>

#include "Export.hpp"
#include "Value.hpp"

struct lua_State;

namespace rtype::luau {
/// @brief Context of a single call to a Function, given to its handler.
///
/// Holds the arguments of the call, in order. Move-only: a context belongs to one call.
class RTYPE_LUAU_API CallContext {
  public:
    /// @brief Creates an empty context.
    CallContext() = default;

    /// @brief Destroys the context.
    ~CallContext() = default;

    /// @brief Copy is deleted: a context belongs to a single call.
    CallContext(const CallContext&) = delete;

    /// @brief Copy is deleted: a context belongs to a single call.
    CallContext& operator=(const CallContext&) = delete;

    /// @brief Takes over another context.
    CallContext(CallContext&&) = default;

    /// @brief Takes over another context.
    /// @return A reference to this context.
    CallContext& operator=(CallContext&&) = default;

    /// @brief Appends an argument after the ones already added.
    /// @param value The argument to add.
    void addArg(Value value);

    /// @brief Gets the number of arguments of the call.
    /// @return The number of arguments added so far.
    [[nodiscard]] std::size_t argCount() const;

    /// @brief Pushes every argument onto the stack of a Lua state, in the order they were added.
    /// @param state The Lua state to push onto; must not be null.
    /// Grows the stack as needed, so the caller does not have to make room for the arguments.
    /// @return The number of arguments pushed.
    /// @throws exceptions::StackOverflow If the stack cannot grow enough to hold every argument. Nothing is
    ///         pushed then.
    /// @throws exceptions::NotImplemented If an argument holds a type that cannot be pushed yet (see
    ///         `Value::push`). The arguments pushed before it stay on the stack.
    std::size_t pushArgs(lua_State* state) const;

  private:
    std::vector<Value> _args;  ///< The arguments of the call, in order.
};
}  // namespace rtype::luau
