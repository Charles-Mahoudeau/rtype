/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CallResult
*/

#include "CallResult.hpp"

#include <lua.h>

#include <cstddef>
#include <span>
#include <utility>

#include "Result.hpp"
#include "Value.hpp"

namespace rtype::luau {
Result<CallResult> CallResult::fromStack(lua_State* state, const std::size_t count) {
    CallResult result{};

    const int intCount{static_cast<int>(count)};

    // Read from the deepest value up, so the values keep the order they were pushed in.
    for (int index = -intCount; index < 0; ++index) {
        auto value = Value::fromStack(state, index);
        if (!value) {
            lua_pop(state, intCount);
            return forwardError<CallResult>(std::move(value));
        }
        result.addValue(std::move(*value));
    }
    lua_pop(state, intCount);
    return result;
}

std::span<const Value> CallResult::values() const noexcept { return _values; }

void CallResult::addValue(Value value) { _values.push_back(std::move(value)); }
}  // namespace rtype::luau
