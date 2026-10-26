/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FunctionHelper
*/

#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <string_view>
#include <type_traits>
#include <variant>

#include "rtype/luau/Export.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Value.hpp"

namespace rtype::luau::detail {
/// @brief Tells whether a type is one of the alternatives of a `std::variant`; only specialized for `std::variant`.
/// @tparam T The type to look for.
/// @tparam Variant The `std::variant` to search.
template <typename T, typename Variant>
inline constexpr bool kIsAlternative = false;

/// @brief Specialization for `std::variant<Ts...>`: true if @p T is exactly one of `Ts`.
template <typename T, typename... Ts>
inline constexpr bool kIsAlternative<T, std::variant<Ts...>> = (std::is_same_v<T, Ts> || ...);

/// @brief Describes a handler parameter: a plain value, or a `std::optional` of one (an optional argument).
/// @tparam T The parameter type, as written in the signature of the handler.
template <typename T>
struct ParamTraits {
    using Plain = std::remove_cvref_t<T>;  ///< The parameter without cv-qualifiers and reference.
    using ValueType = Plain;               ///< The type the Lua argument must hold.
    using ReadType = const Plain&;         ///< What reading the argument gives: a reference to the held value.
    static constexpr bool kIsOptional = false;
};

/// @brief Specialization for `std::optional<U>`: the argument may be missing or `nil`, and otherwise holds a `U`.
template <typename T>
    requires(std::is_same_v<std::remove_cvref_t<T>, std::optional<typename std::remove_cvref_t<T>::value_type>>)
struct ParamTraits<T> {
    using Plain = std::remove_cvref_t<T>;
    using ValueType = Plain::value_type;
    using ReadType = Plain;  ///< A copy of the held value, or an empty optional.
    static constexpr bool kIsOptional = true;
};

/// @brief Describes the signature of a `std::function`.
/// @tparam T The type to describe; only specialized for `std::function`.
template <typename T>
struct FunctionTraits;

/// @brief Describes the signature of a `std::function<R(Args...)>`.
/// @tparam R Return type.
/// @tparam Args Parameter types.
template <typename R, typename... Args>
struct FunctionTraits<std::function<R(Args...)>> {
    using ReturnType = R;  ///< Return type of the function.

    static constexpr std::size_t kArgCount = sizeof...(Args);  ///< Number of parameters.

    /// @brief Whether every parameter holds, without optional, cv-qualifiers and reference, an alternative of
    ///        @p Variant.
    /// @tparam Variant The `std::variant` the parameters must belong to.
    template <typename Variant>
    static constexpr bool kArgsInVariant = (kIsAlternative<typename ParamTraits<Args>::ValueType, Variant> && ...);

    /// @brief Whether every parameter is a value, `const&`, or an optional of those (no `&` or `&&`).
    static constexpr bool kArgsByValueOrConstRef =
        ((!std::is_reference_v<Args> ||
          (std::is_lvalue_reference_v<Args> && std::is_const_v<std::remove_reference_t<Args>>)) &&
         ...);

    /// @brief Number of leading parameters that are not optional, which Lua must pass.
    static constexpr std::size_t kRequiredCount = [] {
        constexpr std::array<bool, sizeof...(Args)> isOptional{ParamTraits<Args>::kIsOptional...};
        std::size_t count = 0;
        while (count < isOptional.size() && !isOptional.at(count)) {
            ++count;
        }
        return count;
    }();

    /// @brief Whether no required parameter follows an optional one.
    static constexpr bool kOptionalsTrailing = [] {
        constexpr std::array<bool, sizeof...(Args)> isOptional{ParamTraits<Args>::kIsOptional...};
        for (std::size_t i = kRequiredCount; i < isOptional.size(); ++i) {
            if (!isOptional.at(i)) {
                return false;
            }
        }
        return true;
    }();
};

/// @brief Gives the Luau name of the type of a `Value` alternative, as `typeof` would.
/// @tparam T One of the alternatives of `Value::Variant`.
template <typename T>
[[nodiscard]] constexpr std::string_view luaTypeName() noexcept;

/// @brief Gives the Luau name of the type a `Value` currently holds.
/// @param value The value to describe.
/// @return The name, as `typeof` would give it.
[[nodiscard]] RTYPE_LUAU_API std::string_view luaTypeNameOf(const Value& value) noexcept;
}  // namespace rtype::luau::detail

#define RTYPE_FUNCTIONHELPER_INCLUDING_TPP
#include "FunctionHelper.tpp"  // IWYU pragma: export
#undef RTYPE_FUNCTIONHELPER_INCLUDING_TPP
