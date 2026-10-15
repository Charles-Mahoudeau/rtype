/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentTraits
*/

#pragma once

#include <string_view>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"

namespace rtype::ecs {
namespace detail {
/// @brief Checks that `T` can be a component and gathers its description.
template <typename T, typename... Fields>
[[nodiscard]] constexpr TypeDescription<sizeof...(Fields)> describeComponent(std::string_view name, Fields... fields);
}  // namespace detail

/// @brief True for a type declared with RTYPE_ECS_COMPONENT.
template <typename T>
concept Component = Described<T> && TypeTraits<T>::kKind == TypeKind::kComponent;

/// @brief Everything known at compile time about a component declared with RTYPE_ECS_COMPONENT: what every declared
/// type has (TypeTraits: name, hashes, fields, hooks), and how to describe it as a component.
template <Component T>
struct ComponentTraits : TypeTraits<T> {
    /// @return A ComponentInfo with the type's description; storage, id, presence, replication and layout hash are
    /// left for the registry to set.
    [[nodiscard]] static ComponentInfo makeInfo();
};
}  // namespace rtype::ecs

#include "ComponentTraits.tpp"

/// @brief Declares a component: its stable name and the fields to reflect, written once next to the struct.
///
/// @details Use it right after the struct, in the same namespace, ending with a semicolon. Every listed field must
/// have a FieldTypeOf mapping (scalars, a `std::array` of them, or types mapped by a specialization).
/// Members that are not listed are stored but not reflected. A struct without fields is a tag.
/// @code
/// struct Health {
///     std::int32_t value;
///     std::int32_t max;
/// };
/// RTYPE_ECS_COMPONENT(Health, "rtype.Health", value, max);
///
/// struct Dead {};
/// RTYPE_ECS_COMPONENT(Dead, "rtype.Dead");
/// @endcode
/// @param Type The struct, as named in the current namespace.
/// @param Name Its stable name, a string literal such as "rtype.Health".
/// @param ... The fields to reflect.
#define RTYPE_ECS_COMPONENT(Type, Name, ...)                                                                  \
    RTYPE_ECS_DETAIL_DESCRIBE(::rtype::ecs::detail::describeComponent, Type, Name __VA_OPT__(, ) __VA_ARGS__) \
    static_assert(true, "RTYPE_ECS_COMPONENT must be followed by a semicolon")
