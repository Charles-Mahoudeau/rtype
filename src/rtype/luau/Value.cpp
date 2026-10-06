/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Value
*/

#include "Value.hpp"

#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <variant>

#include "VariantHelper.hpp"

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
    using Ret = std::optional<std::int32_t>;

    return std::visit(Overload{
                          [](const double v) -> Ret {
                              if (!checkLimits(v, kLimits)) {
                                  return std::nullopt;
                              }
                              return static_cast<T>(v);
                          },
                          [](const std::int64_t v) -> Ret {
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

bool Value::isNumeric() const noexcept {
    return std::holds_alternative<double>(_variant) || std::holds_alternative<std::int64_t>(_variant);
}

std::optional<std::int32_t> Value::toInt32() const {
    return convertSafe<std::int32_t>(_variant);
}

std::optional<std::uint32_t> Value::toUInt32() const {
    return convertSafe<std::uint32_t>(_variant);
}
}  // namespace rtype::luau
