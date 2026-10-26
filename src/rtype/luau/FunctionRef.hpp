/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FunctionRef
*/

#pragma once

#include <memory>

#include "Export.hpp"
#include "Ref.hpp"
#include "Result.hpp"

struct lua_State;

namespace rtype::luau {
/// @brief Copyable handle on a Luau function, kept alive through a registry reference.
///
/// A pure handle: it can be held, copied and pushed back to Lua, but not called. Turn it into a callable
/// `Function` with `Function::fromRef`. Every copy shares the same registry reference, which is released with the
/// last copy. The referenced `lua_State` must outlive the handle.
class RTYPE_LUAU_API FunctionRef {
  public:
    /// @brief Reads a function from the stack of a Lua state.
    ///
    /// Copies the value without popping it, and leaves the stack untouched.
    /// @param state The Lua state to read from; must not be null.
    /// @param stackIndex Stack index of the function to read (defaults to the top). It must be a valid index
    ///        of @p state's stack.
    /// @return A handle on the function, or a Failure of kind `kTypeMismatch` if the value is not a function.
    /// @throws exceptions::InvalidState If @p state is null.
    [[nodiscard]] static Result<FunctionRef> fromStack(lua_State* state, int stackIndex = Ref::kLuaStackTop);

    /// @brief Destroys the handle; the registry reference is released with the last copy.
    ~FunctionRef() noexcept = default;
    /// @brief Copy constructor: the copy shares the registry reference.
    FunctionRef(const FunctionRef&) noexcept = default;
    /// @brief Copy assignment: the handle then shares the registry reference of @p other.
    FunctionRef& operator=(const FunctionRef&) noexcept = default;
    /// @brief Move constructor.
    FunctionRef(FunctionRef&&) noexcept = default;
    /// @brief Move assignment.
    FunctionRef& operator=(FunctionRef&&) noexcept = default;

    /// @brief Gives access to the registry reference to the function.
    /// @return The reference.
    /// @throws exceptions::InvalidRef If the handle was moved from.
    [[nodiscard]] const Ref& ref() const;

    /// @brief Pushes the function onto the stack of a Lua state.
    ///
    /// Pushes exactly one value on success and nothing on failure. Does not grow the stack: the caller makes sure
    /// there is room for one more slot (`lua_checkstack`).
    /// @param state The Lua state to push onto; must not be null.
    /// @return Nothing on success, or a Failure of kind `kRuntime` if @p state belongs to another Lua state than the
    ///         function.
    /// @throws exceptions::InvalidState If @p state is null.
    /// @throws exceptions::InvalidRef If the handle was moved from.
    [[nodiscard]] Result<void> push(lua_State* state) const;

  private:
    /// @brief Wraps a reference to a function.
    /// @param ref Registry reference to the function.
    explicit FunctionRef(Ref ref);

    std::shared_ptr<Ref> _ref;  ///< Registry reference to the function, shared by every copy of the handle.
};
}  // namespace rtype::luau
