/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Stack
*/

#pragma once

#include <cstddef>

#include "rtype/luau/CallContext.hpp"
#include "rtype/luau/Export.hpp"
#include "rtype/luau/FunctionRef.hpp"
#include "rtype/luau/Ref.hpp"
#include "rtype/luau/Result.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Userdata.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"

struct lua_State;

/// The functions that read values from the stack of a Lua state and push them back. Users of `luau` never hold a raw
/// `lua_State*`: these are for the native closures, the usertypes and the tests. The functions that need the private
/// members of a handle (`readTable`, `pushTable`, `readUserdata`, `pushUserdata`, `readFunctionRef`,
/// `pushFunctionRef`) are declared next to the handle.
namespace rtype::luau::detail {
/// @brief Reads a value from the stack of a Lua state, as the matching Value alternative.
///
/// Copies the value without popping it, and leaves the stack untouched. The reverse of `pushValue()`.
/// @param state The Lua state to read from; must not be null.
/// @param stackIndex Stack index of the value to read (defaults to the top). It must be a valid index of @p state's
///        stack.
/// @return A Value holding a copy of the Luau value, or a Failure of kind `kTypeMismatch` if the value has a type
///         that cannot be read yet (thread, buffer, light userdata).
[[nodiscard]] RTYPE_LUAU_API Result<Value> readValue(lua_State* state, int stackIndex = Ref::kLuaStackTop);

/// @brief Reads the value on top of the stack of a Lua state and pops it.
///
/// Same as `readValue()` on the top of the stack, followed by a pop on success. On failure, the stack is left
/// untouched: the value stays on top.
/// @param state The Lua state to read from; must not be null, and its stack must not be empty.
/// @return A Value holding a copy of the Luau value, or a Failure of kind `kTypeMismatch` if the value has a type
///         that cannot be read yet (thread, buffer, light userdata).
[[nodiscard]] RTYPE_LUAU_API Result<Value> popValue(lua_State* state);

/// @brief Pushes a value onto the stack of a Lua state, as the matching Luau type.
///
/// Pushes exactly one value on success and nothing on failure. Does not grow the stack: the caller makes sure there
/// is room for one more slot (`lua_checkstack`).
/// @param state The Lua state to push onto; must not be null.
/// @param value The value to push.
/// @return Nothing on success, or a Failure of kind `kRuntime` if the Value holds a `Table` of another Lua state than
///         @p state, or of kind `kOutOfMemory` if a string cannot be allocated.
/// @throws exceptions::InvalidState If @p state is null.
[[nodiscard]] RTYPE_LUAU_API Result<void> pushValue(lua_State* state, const Value& value);

/// @brief Reads a range of the stack of a Lua state into a pack, without popping it.
///
/// The values are read from the deepest up, so the value at @p first becomes the first value of the pack.
/// @param state The Lua state to read from; must not be null.
/// @param first Absolute (positive) stack index of the first value to read.
/// @param count Number of values to read; the stack must hold every index from @p first to `first + count - 1`.
/// @return A pack holding a copy of the values, or a Failure of kind `kTypeMismatch` if one of them has a type that
///         cannot be read yet (see `readValue()`). Its message starts with the 1-based position of that value in the
///         pack (`value #2: ...`).
[[nodiscard]] RTYPE_LUAU_API Result<ValuePack> readValuePack(lua_State* state, int first, std::size_t count);

/// @brief Reads the values on top of the stack of a Lua state into a pack, and pops them.
///
/// Same as `readValuePack()` on the top @p count values, followed by a pop on success. On failure, the stack is left
/// untouched: the values stay on top.
/// @param state The Lua state to read from; must not be null.
/// @param count Number of values to read from the top of @p state's stack. The stack must hold at least that many
///        values.
/// @return A pack holding a copy of the values, the deepest first, or a Failure of kind `kTypeMismatch` if one of
///         them has a type that cannot be read yet (see `readValuePack()`).
[[nodiscard]] RTYPE_LUAU_API Result<ValuePack> popValuePack(lua_State* state, std::size_t count);

/// @brief Pushes every value of a pack onto the stack of a Lua state, in the order they were added.
///
/// Grows the stack as needed, so the caller does not have to make room for the values.
/// @param state The Lua state to push onto; must not be null.
/// @param pack The pack to push.
/// @return The number of values pushed, or a Failure of kind `kStackOverflow` if the stack cannot grow enough to hold
///         every value, or the failure of `pushValue()` for a value that cannot be pushed. Nothing stays on the stack
///         on failure.
/// @throws exceptions::InvalidState If @p state is null.
[[nodiscard]] RTYPE_LUAU_API Result<std::size_t> pushValuePack(lua_State* state, const ValuePack& pack);

/// @brief Reads the arguments of a call to a native closure from the stack of a Lua state, without popping them.
///
/// Meant to be called from a C function: every value of the stack, from index 1 up, is an argument.
/// @param state The Lua state calling the closure; must not be null.
/// @return A context holding a copy of the arguments in order, or a Failure of kind `kTypeMismatch` if one of them
///         has a type that cannot be read yet (see `readValuePack()`).
[[nodiscard]] RTYPE_LUAU_API Result<CallContext> readCallContext(lua_State* state);
}  // namespace rtype::luau::detail
