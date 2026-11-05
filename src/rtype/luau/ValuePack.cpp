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
#include "Exceptions.hpp"
#include "Failure.hpp"
#include "Result.hpp"
#include "Value.hpp"
#include "detail/Stack.hpp"

namespace rtype::luau {
Result<ValuePack> detail::readValuePack(lua_State* state, const int first, const std::size_t count) {
    ValuePack pack{};

    for (std::size_t position = 0; position < count; ++position) {
        auto value = readValue(state, first + static_cast<int>(position));
        if (!value) {
            return Failure{ErrorKind::kTypeMismatch,
                           std::format("value #{}: {}", position + 1, value.error().message())};
        }
        pack.add(std::move(*value));
    }
    return pack;
}

Result<ValuePack> detail::popValuePack(lua_State* state, const std::size_t count) {
    const int intCount{static_cast<int>(count)};
    auto pack = readValuePack(state, lua_gettop(state) - intCount + 1, count);

    if (!pack) {
        return forwardError<ValuePack>(std::move(pack));
    }
    lua_pop(state, intCount);
    return pack;
}

std::span<const Value> ValuePack::values() const noexcept { return _values; }

std::size_t ValuePack::size() const noexcept { return _values.size(); }

void ValuePack::add(Value value) { _values.push_back(std::move(value)); }

Result<std::size_t> detail::pushValuePack(lua_State* state, const ValuePack& pack) {
    if (state == nullptr) {
        throw exceptions::InvalidState{};
    }
    if (pack.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        lua_checkstack(state, static_cast<int>(pack.size())) == 0) {
        return Failure{ErrorKind::kStackOverflow, "Lua stack overflow"};
    }
    const int top{lua_gettop(state)};
    for (const auto& value : pack.values()) {
        if (auto pushed = pushValue(state, value); !pushed) {
            lua_settop(state, top);
            return forwardError<std::size_t>(std::move(pushed));
        }
    }
    return pack.size();
}
}  // namespace rtype::luau
