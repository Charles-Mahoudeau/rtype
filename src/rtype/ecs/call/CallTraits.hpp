/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CallTraits
*/

#pragma once

#include <string_view>

#include "rtype/ecs/type/TypeTraits.hpp"

namespace rtype::ecs {
namespace detail {
/// @brief Checks that `T` can be the arguments or the result of an engine call and gathers its description.
template <typename T, typename... Fields>
[[nodiscard]] constexpr TypeDescription<sizeof...(Fields)> describeCallData(std::string_view name, Fields... fields);
}  // namespace detail

/// @brief True for a type declared with RTYPE_ECS_CALL_DATA: the arguments or the result of an engine call.
template <typename T>
concept CallData = Described<T> && TypeTraits<T>::kKind == TypeKind::kCallData;
}  // namespace rtype::ecs

#define RTYPE_ECS_CALL_TRAITS_INCLUDING_TPP
#include "CallTraits.tpp"  // IWYU pragma: export
#undef RTYPE_ECS_CALL_TRAITS_INCLUDING_TPP

/// @brief Declares the arguments or the result of an engine call: a stable name, and the fields a script gives or
/// receives, written once next to the type.
///
/// @details Use it right after the type, in the same namespace, ending with a semicolon. Call data is
/// standard-layout and default-constructible, and moving or destroying it does not throw: a script runtime builds it
/// from its defaults and the fields the script names (in a CallBuffer), then the call reads it there. Only the listed
/// fields can be set from a script; the others keep their default. A field is a value, an ecs::String (255 bytes at
/// most) or, for a longer text or a list, a `std::string` or a `std::vector` of plain values: such call data owns
/// memory, so it is never copied as bytes.
/// @code
/// struct LoadSound {
///     rtype::core::asset::AssetId path;
///     bool stream = false;
/// };
/// RTYPE_ECS_CALL_DATA(LoadSound, "rtype.audio.LoadSound", path, stream);
///
/// struct Ask {
///     std::string prompt;  // a text of any length
/// };
/// RTYPE_ECS_CALL_DATA(Ask, "rtype.agent.Ask", prompt);
/// @endcode
/// @param Type The type, as named in the current namespace.
/// @param Name Its stable name, a string literal such as "rtype.audio.LoadSound".
/// @param ... The fields to reflect, if any.
#define RTYPE_ECS_CALL_DATA(Type, Name, ...)                                                                 \
    RTYPE_ECS_DETAIL_DESCRIBE(::rtype::ecs::detail::describeCallData, Type, Name __VA_OPT__(, ) __VA_ARGS__) \
    static_assert(true, "RTYPE_ECS_CALL_DATA must be followed by a semicolon")
