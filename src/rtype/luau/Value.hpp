/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Value
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <optional>
#include <string>
#include <variant>

#include "Export.hpp"
#include "Ref.hpp"
#include "Table.hpp"

struct lua_State;

namespace rtype::luau {
/// @brief A dynamically typed Luau value, held by value on the C++ side.
///
/// Wraps a `std::variant` of the Luau types that are currently supported. Use `is<T>()` to inspect
/// the held type, `as<T>()` or `tryAs<T>()` to access it, and `push()` to hand it over to a Lua state.
class RTYPE_LUAU_API Value {
  public:
    /// @brief Luau `nil`.
    using Nil = std::monostate;
    /// @brief Luau native vector type (`vector`), three components wide (checked against `LUA_VECTOR_SIZE`).
    using Vector = glm::vec3;
    /// @brief The set of Luau types a Value can hold.
    ///
    /// See https://luau.org/api/#type-inspection. Numbers are all `double` for now: 64-bit integers
    /// come with a later Luau version. Not implemented types (yet): Function, Userdata, Thread,
    /// Buffer, Class.
    using Variant = std::variant<Nil, bool, double, Vector, std::string, Table>;

    /// @brief Reads a value from the stack of a Lua state, as the matching Value alternative.
    ///
    /// Copies the value without popping it, and leaves the stack untouched. The reverse of `push()`.
    /// @param state The Lua state to read from; must not be null.
    /// @param stackIndex Stack index of the value to read (defaults to the top). It must be a valid index
    ///        of @p state's stack.
    /// @return A Value holding a copy of the Luau value.
    /// @throws exceptions::NotImplemented If the value has a type that cannot be read yet (table,
    ///         function, userdata, thread, buffer).
    [[nodiscard]] static Value fromStack(lua_State* state, int stackIndex = Ref::kLuaStackTop);

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

    /// @brief Pushes the held value onto the stack of a Lua state, as the matching Luau type.
    ///
    /// Pushes exactly one value on success and nothing when it throws. Does not grow the stack: the
    /// caller makes sure there is room for one more slot (`lua_checkstack`).
    /// @param state The Lua state to push onto; must not be null.
    /// @throws exceptions::NotImplemented If the Value holds a type that cannot be pushed yet (`Table`).
    void push(lua_State* state) const;

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
    /// @return A const pointer to the held value, or `nullptr` if the Value does not hold a `T`.
    template <typename T>
    [[nodiscard]] const T* tryAs() const noexcept;
    /// @brief Accesses the held value as a `T`, without throwing.
    /// @tparam T One of the alternatives of `Variant`.
    /// @return A pointer to the held value, or `nullptr` if the Value does not hold a `T`.
    template <typename T>
    [[nodiscard]] T* tryAs() noexcept;

    /// @brief Checks whether the Value holds a number (a `double`, the only numeric type for now).
    /// @return `true` if the Value is numeric.
    [[nodiscard]] bool isNumeric() const noexcept;
    /// @brief Converts the Value to a 32-bit signed integer.
    /// @return The number truncated towards zero, or `std::nullopt` if the Value is not numeric, is
    ///         NaN, or lies outside the range of `std::int32_t` (checked before truncating).
    [[nodiscard]] std::optional<std::int32_t> toInt32() const;
    /// @brief Converts the Value to a 32-bit unsigned integer.
    /// @return The number truncated towards zero, or `std::nullopt` if the Value is not numeric, is
    ///         NaN, or lies outside the range of `std::uint32_t` (checked before truncating, so
    ///         `-0.5` is rejected).
    [[nodiscard]] std::optional<std::uint32_t> toUInt32() const;

  private:
    Variant _variant;  ///< The held value.
};
}  // namespace rtype::luau

// NOLINTNEXTLINE(*-include-cleaner)
#include "Value.tpp"
