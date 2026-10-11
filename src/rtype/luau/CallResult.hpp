/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CallResult
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
/// @brief Result of a single call to a Function, returned by its handler and by `Function::call`.
///
/// Holds the values returned by the call, in the order of the `return` statement. Move-only: a result
/// belongs to one call.
class RTYPE_LUAU_API CallResult {
  public:
    /// @brief Reads the values on top of the stack of a Lua state into a result, and pops them.
    ///
    /// The values are read from the deepest up, so the first one pushed becomes the first value of the
    /// result. The values are popped in every case, whether reading them succeeds or not.
    /// @param state The Lua state to read from; must not be null.
    /// @param count Number of values to read from the top of @p state's stack. The stack must hold at least
    ///        that many values.
    /// @return A CallResult holding a copy of the values, or a Failure of kind `kTypeMismatch` if one of
    ///         them has a type that cannot be read yet (table, function, userdata, thread, buffer).
    [[nodiscard]] static Result<CallResult> fromStack(lua_State* state, std::size_t count);

    /// @brief Creates an empty result.
    CallResult() = default;

    /// @brief Destroys the result.
    ~CallResult() = default;

    /// @brief Copy is deleted: a result belongs to a single call.
    CallResult(const CallResult&) = delete;

    /// @brief Copy is deleted: a result belongs to a single call.
    CallResult& operator=(const CallResult&) = delete;

    /// @brief Takes over another result.
    CallResult(CallResult&&) = default;

    /// @brief Takes over another result.
    /// @return A reference to this result.
    CallResult& operator=(CallResult&&) = default;

    /// @brief Gets the values held by the result.
    /// @return A view over the values, in the order they were added.
    [[nodiscard]] std::span<const Value> values() const noexcept;

    /// @brief Appends a value after the ones already held.
    /// @param value The value to add.
    void addValue(Value value);

  private:
    std::vector<Value> _values;  ///< Values returned by the call, in order.
};
}  // namespace rtype::luau
