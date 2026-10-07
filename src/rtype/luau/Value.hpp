/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Value
*/

#pragma once

#include <luaconf.h>

#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <optional>
#include <string>
#include <variant>

#include "Export.hpp"
#include "Table.hpp"

namespace rtype::luau {
/// @brief A dynamically typed Luau value, held by value on the C++ side.
///
/// Wraps a `std::variant` of the Luau types that are currently supported. Use `is<T>()` to inspect
/// the held type and `as<T>()` to access it.
class RTYPE_LUAU_API Value {
  public:
    /// @brief Luau `nil`.
    using Nil = std::monostate;
#if LUA_VECTOR_SIZE == 3
    /// @brief Luau native vector type (`vector`), matching the configured `LUA_VECTOR_SIZE`.
    using Vector = glm::vec3;
#elif LUA_VECTOR_SIZE == 4
    using Vector = glm::vec4;
#else
#error "Invalid LUA_VECTOR_SIZE"
#endif
    /// @brief The set of Luau types a Value can hold.
    ///
    /// See https://luau.org/api/#type-inspection. Not implemented types (yet): Function, Userdata,
    /// Thread, Buffer, Class.
    using Variant = std::variant<Nil, bool, double, std::int64_t, Vector, std::string, Table>;

    /// @brief Constructs a Value from any of the supported types.
    /// @param v The variant to hold.
    // ReSharper disable once CppNonExplicitConvertingConstructor
    // NOLINTNEXTLINE(*-explicit-constructor)
    Value(Variant v) noexcept;
    /// @brief Destroys the Value.
    ~Value() noexcept;
    /// @brief Copy constructor.
    Value(const Value&) noexcept;
    /// @brief Copy assignment.
    Value& operator=(const Value&) noexcept;
    /// @brief Move constructor.
    Value(Value&&) noexcept;
    /// @brief Move assignment.
    Value& operator=(Value&&) noexcept;

    /// @brief Checks whether the Value currently holds a `T`.
    /// @tparam T One of the alternatives of `Variant`.
    /// @return `true` if the held type is exactly `T`.
    template <typename T>
    [[nodiscard]] bool is() const noexcept;
    /// @brief Accesses the held value as a `T`.
    /// @tparam T One of the alternatives of `Variant`.
    /// @return A const reference to the held value.
    /// @throws std::bad_variant_access If the Value does not hold a `T`.
    template <typename T>
    [[nodiscard]] const T& as() const;
    /// @brief Accesses the held value as a `T`.
    /// @tparam T One of the alternatives of `Variant`.
    /// @return A reference to the held value.
    /// @throws std::bad_variant_access If the Value does not hold a `T`.
    template <typename T>
    [[nodiscard]] T& as();
    /// @brief Accesses the held value as a `T`, without throwing.
    /// @tparam T One of the alternatives of `Variant`.
    /// @return A const reference to the held value, or `std::nullopt` if the Value does not hold a `T`.
    template<typename T>
    [[nodiscard]] const T* tryAs() const noexcept;
    /// @brief Accesses the held value as a `T`, without throwing.
    /// @tparam T One of the alternatives of `Variant`.
    /// @return A reference to the held value, or `std::nullopt` if the Value does not hold a `T`.
    template<typename T>
    [[nodiscard]] T* tryAs() noexcept;

    /// @brief Checks whether the Value holds a number (`double` or `std::int64_t`).
    /// @return `true` if the Value is numeric.
    [[nodiscard]] bool isNumeric() const noexcept;
    /// @brief Converts the Value to a 32-bit signed integer.
    /// @return The converted value (truncated towards zero if it is a `double`), or `std::nullopt`
    ///         if the Value is not numeric or does not fit in `std::int32_t`.
    [[nodiscard]] std::optional<std::int32_t> toInt32() const;
    /// @brief Converts the Value to a 32-bit unsigned integer.
    /// @return The converted value (truncated towards zero if it is a `double`), or `std::nullopt`
    ///         if the Value is not numeric or does not fit in `std::uint32_t`.
    [[nodiscard]] std::optional<std::uint32_t> toUInt32() const;

  private:
    Variant _variant;  ///< The held value.
};
}  // namespace rtype::luau

// NOLINTNEXTLINE(*-include-cleaner)
#include "Value.tpp"
