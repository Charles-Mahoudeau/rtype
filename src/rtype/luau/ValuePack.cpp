/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ValuePack
*/

#include "ValuePack.hpp"

#include <lua.h>

#include <cstddef>
#include <format>
#include <limits>
#include <span>
#include <utility>

#include "ErrorKind.hpp"
#include "Failure.hpp"
#include "Result.hpp"
#include "Value.hpp"

namespace rtype::luau {
Result<ValuePack> ValuePack::fromStack(lua_State* state, const int first, const std::size_t count) {
    ValuePack pack{};

    for (std::size_t position = 0; position < count; ++position) {
        auto value = Value::fromStack(state, first + static_cast<int>(position));
        if (!value) {
            return Failure{ErrorKind::kTypeMismatch,
                           std::format("value #{}: {}", position + 1, value.error().message())};
        }
        pack.add(std::move(*value));
    }
    return pack;
}

Result<ValuePack> ValuePack::pop(lua_State* state, const std::size_t count) {
    const int intCount{static_cast<int>(count)};
    auto pack = fromStack(state, lua_gettop(state) - intCount + 1, count);

    if (!pack) {
        return forwardError<ValuePack>(std::move(pack));
    }
    lua_pop(state, intCount);
    return pack;
}

std::span<const Value> ValuePack::values() const noexcept { return _values; }

std::size_t ValuePack::size() const noexcept { return _values.size(); }

void ValuePack::add(Value value) { _values.push_back(std::move(value)); }

Result<std::size_t> ValuePack::push(lua_State* state) const {
    if (_values.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        lua_checkstack(state, static_cast<int>(_values.size())) == 0) {
        return Failure{ErrorKind::kStackOverflow, "Lua stack overflow"};
    }
    const int top{lua_gettop(state)};
    for (const auto& value : _values) {
        if (auto pushed = value.push(state); !pushed) {
            lua_settop(state, top);
            return forwardError<std::size_t>(std::move(pushed));
        }
    }
    return _values.size();
}
}  // namespace rtype::luau
