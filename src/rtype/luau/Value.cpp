/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Value
*/

#include "Value.hpp"

#include <lua.h>
#include <luaconf.h>

#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "ErrorKind.hpp"
#include "Failure.hpp"
#include "Ref.hpp"
#include "Result.hpp"
#include "Table.hpp"
#include "VariantHelper.hpp"

static_assert(LUA_VECTOR_SIZE == 3, "rtype::luau::Value::Vector assumes 3-component Luau vectors");

namespace {
template <typename T, typename U>
[[nodiscard]] bool checkLimits(T value, std::numeric_limits<U> limits) {
    if constexpr (std::numeric_limits<U>::is_bounded) {
        return value >= limits.min() && value <= limits.max();
    }
    return true;
}

template <typename T>
[[nodiscard]] std::optional<T> convertSafe(const rtype::luau::Value::Variant& variant) {
    static constexpr std::numeric_limits<T> kLimits;
    using Ret = std::optional<T>;

    // over-engineered, for now, awaiting the int64 update
    return std::visit(Overload{
                          [](const double v) -> Ret {
                              if (!checkLimits(v, kLimits)) {
                                  return std::nullopt;
                              }
                              return static_cast<T>(v);
                          },
                          [](const auto&) -> Ret { return std::nullopt; },
                      },
                      variant);
}
}  // namespace

namespace rtype::luau {
Result<Value> Value::fromStack(lua_State* state, const int stackIndex) {
    switch (const int type = lua_type(state, stackIndex)) {
        case LUA_TNIL:
            return Value{Nil{}};
        case LUA_TBOOLEAN:
            return Value{lua_toboolean(state, stackIndex) != 0};
        case LUA_TNUMBER:
            return Value{lua_tonumber(state, stackIndex)};
        case LUA_TVECTOR: {
            const float* v{lua_tovector(state, stackIndex)};
            // NOLINTNEXTLINE(*-pro-bounds-pointer-arithmetic)
            return Value{Vector{v[0], v[1], v[2]}};
        }
        case LUA_TSTRING: {
            std::size_t length{};
            const char* str{lua_tolstring(state, stackIndex, &length)};
            return Value{std::string{str, length}};
        }
        case LUA_TTABLE: {
            auto table = Table::fromStack(state, stackIndex);
            if (!table) {
                return forwardError<Value>(std::move(table));
            }
            return Value{std::move(*table)};
        }
        default:
            return Failure{ErrorKind::kTypeMismatch,
                           std::format("cannot read a value of type '{}' yet", lua_typename(state, type))};
    }
}

Result<Value> Value::pop(lua_State* state) {
    auto value = fromStack(state, Ref::kLuaStackTop);

    if (!value) {
        return forwardError<Value>(std::move(value));
    }
    lua_pop(state, 1);
    return value;
}

Value::Value(Variant v) noexcept : _variant{std::move(v)} {}
Value::~Value() noexcept = default;
Value::Value(const Value&) noexcept = default;
Value& Value::operator=(const Value&) noexcept = default;
Value::Value(Value&&) noexcept = default;
Value& Value::operator=(Value&&) noexcept = default;

void Value::push(lua_State* state) const {
    std::visit(Overload{
                   [state](const Nil) { lua_pushnil(state); },
                   [state](const bool v) { lua_pushboolean(state, v); },
                   [state](const double v) { lua_pushnumber(state, v); },
                   [state](const Vector v) { lua_pushvector(state, v.x, v.y, v.z); },
                   [state](const std::string& v) { lua_pushlstring(state, v.data(), v.size()); },
                   [state](const Table& v) { v.push(state); },
               },
               _variant);
}

bool Value::isNumeric() const noexcept {
    // for now, we only have double type, but after upgrading Luau version, we'll have int64
    return std::holds_alternative<double>(_variant);
}

std::optional<std::int32_t> Value::toInt32() const { return convertSafe<std::int32_t>(_variant); }

std::optional<std::uint32_t> Value::toUInt32() const { return convertSafe<std::uint32_t>(_variant); }
}  // namespace rtype::luau
