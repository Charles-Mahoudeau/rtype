/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Value
*/

#include "Value.hpp"

#include <lua.h>
#include <luaconf.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "Exceptions.hpp"
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
                   [](auto&) { throw exceptions::NotImplemented{}; },
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
