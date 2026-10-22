/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ValuePack
*/

#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "Export.hpp"
#include "Result.hpp"
#include "Value.hpp"

struct lua_State;

namespace rtype::luau {
/// @brief Ordered sequence of Values exchanged with a Lua stack, such as the arguments or the results of a call.
///
/// The runtime counterpart of a Luau type pack: `push()` hands every value over to a Lua state, `fromStack()` reads
/// a range of a Lua stack back into a pack, and `pop()` reads the top of a Lua stack and pops it. `Function::call`
/// takes its arguments and returns its results as packs, and a `Function::Handler` returns its results as one.
class RTYPE_LUAU_API ValuePack {
  public:
    /// @brief Reads a range of the stack of a Lua state into a pack, without popping it.
    ///
    /// The values are read from the deepest up, so the value at @p first becomes the first value of the pack.
    /// @param state The Lua state to read from; must not be null.
    /// @param first Absolute (positive) stack index of the first value to read.
    /// @param count Number of values to read; the stack must hold every index from @p first to
    ///        `first + count - 1`.
    /// @return A pack holding a copy of the values, or a Failure of kind `kTypeMismatch` if one of them has a type
    ///         that cannot be read yet (see `Value::fromStack`). Its message starts with the 1-based position of
    ///         that value in the pack (`value #2: ...`).
    [[nodiscard]] static Result<ValuePack> fromStack(lua_State* state, int first, std::size_t count);

    /// @brief Reads the values on top of the stack of a Lua state into a pack, and pops them.
    ///
    /// Same as `fromStack()` on the top @p count values, followed by a pop on success. On failure, the stack is left
    /// untouched: the values stay on top.
    /// @param state The Lua state to read from; must not be null.
    /// @param count Number of values to read from the top of @p state's stack. The stack must hold at least that
    ///        many values.
    /// @return A pack holding a copy of the values, the deepest first, or a Failure of kind `kTypeMismatch` if one
    ///         of them has a type that cannot be read yet (see `fromStack()`).
    [[nodiscard]] static Result<ValuePack> pop(lua_State* state, std::size_t count);

    /// @brief Creates an empty pack.
    ValuePack() = default;

    /// @brief Destroys the pack.
    ~ValuePack() = default;

    /// @brief Copies another pack and its values.
    ValuePack(const ValuePack&) = default;

    /// @brief Copies another pack and its values.
    /// @return A reference to this pack.
    ValuePack& operator=(const ValuePack&) = default;

    /// @brief Takes over another pack.
    ValuePack(ValuePack&&) noexcept = default;

    /// @brief Takes over another pack.
    /// @return A reference to this pack.
    ValuePack& operator=(ValuePack&&) noexcept = default;

    /// @brief Gets the values held by the pack.
    /// @return A view over the values, in the order they were added.
    [[nodiscard]] std::span<const Value> values() const noexcept;

    /// @brief Gets the number of values held by the pack.
    /// @return The number of values added so far.
    [[nodiscard]] std::size_t size() const noexcept;

    /// @brief Appends a value after the ones already held.
    /// @param value The value to add.
    void add(Value value);

    /// @brief Pushes every value onto the stack of a Lua state, in the order they were added.
    ///
    /// Grows the stack as needed, so the caller does not have to make room for the values.
    /// @param state The Lua state to push onto; must not be null.
    /// @return The number of values pushed, or a Failure of kind `kStackOverflow` if the stack cannot grow enough
    ///         to hold every value, or the failure of `Value::push` for a value that cannot be pushed. Nothing stays
    ///         on the stack on failure.
    [[nodiscard]] Result<std::size_t> push(lua_State* state) const;

  private:
    std::vector<Value> _values;  ///< The values of the pack, in order.
};
}  // namespace rtype::luau
