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
class Function;

/// @brief Copyable handle on a Luau function, kept alive through a registry reference.
///
/// A pure handle: it can be held, copied and pushed back to Lua, but not called. Turn it into a callable
/// `Function` with `Function::fromRef`. Every copy shares the same registry reference, which is released with the
/// last copy. The referenced `lua_State` must outlive the handle.
///
/// @note Naming convention: a `*Ref` type is a bare handle, while a type without the suffix adds operations to it.
///       `Table` is a handle with its operations built in. A function cannot be, because calling it needs
///       `ValuePack`, which includes `Value.hpp`, so the handle stays here and `Function` adds `call` and the native
///       binding.
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

    /// @brief Tells whether the handle holds a registry reference.
    /// @return False if the handle was moved from, true otherwise.
    [[nodiscard]] bool isValid() const noexcept;

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
    friend class Function;

    /// @brief Creates an empty handle, which references nothing until a valid one is assigned to it.
    FunctionRef() noexcept = default;

    /// @brief Wraps a reference to a function.
    /// @param ref Registry reference to the function; the caller makes sure it references a function.
    explicit FunctionRef(Ref ref);

    std::shared_ptr<Ref> _ref;  ///< Registry reference to the function, shared by every copy of the handle.
};
}  // namespace rtype::luau
