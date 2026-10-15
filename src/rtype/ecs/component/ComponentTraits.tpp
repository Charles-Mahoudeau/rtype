/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentTraits
*/

#include <string_view>
#include <type_traits>

namespace rtype::ecs {
namespace detail {
template <typename T, typename... Fields>
constexpr TypeDescription<sizeof...(Fields)> describeComponent(std::string_view name, Fields... fields) {
    static_assert(std::is_aggregate_v<T>,
                  "RTYPE_ECS_COMPONENT: a component must be a plain struct (no constructors, no private members, "
                  "no virtual functions).");
    static_assert(sizeof...(Fields) == 0 || std::is_standard_layout_v<T>,
                  "RTYPE_ECS_COMPONENT: a component with reflected fields must be standard-layout (all members "
                  "public, no reference members, no virtual base).");
    static_assert(std::is_copy_constructible_v<T>, "RTYPE_ECS_COMPONENT: a component must be copyable.");
    static_assert(std::is_nothrow_move_constructible_v<T>, "RTYPE_ECS_COMPONENT: moving a component must not throw.");
    static_assert(std::is_nothrow_destructible_v<T>, "RTYPE_ECS_COMPONENT: destroying a component must not throw.");
    return TypeDescription<sizeof...(Fields)>{.name = name, .kind = TypeKind::kComponent, .fields = {fields...}};
}
}  // namespace detail

template <Component T>
ComponentInfo ComponentTraits<T>::makeInfo() {
    return ComponentInfo{.type = TypeTraits<T>::makeTypeInfo()};
}
}  // namespace rtype::ecs
