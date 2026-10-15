/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ResourceTraits
*/

#include <string_view>
#include <type_traits>

namespace rtype::ecs {
namespace detail {
template <typename T, typename... Fields>
constexpr TypeDescription<sizeof...(Fields)> describeResource(std::string_view name, Fields... fields) {
    static_assert(std::is_object_v<T> && !std::is_array_v<T> && !std::is_const_v<T> && !std::is_volatile_v<T>,
                  "RTYPE_ECS_RESOURCE: a resource must be a plain, non-const object type.");
    static_assert(sizeof...(Fields) == 0 || std::is_standard_layout_v<T>,
                  "RTYPE_ECS_RESOURCE: a resource with reflected fields must be standard-layout (all members public, "
                  "no reference members, no virtual base).");
    static_assert(std::is_nothrow_move_constructible_v<T>, "RTYPE_ECS_RESOURCE: moving a resource must not throw.");
    static_assert(std::is_nothrow_destructible_v<T>, "RTYPE_ECS_RESOURCE: destroying a resource must not throw.");
    return TypeDescription<sizeof...(Fields)>{.name = name, .kind = TypeKind::kResource, .fields = {fields...}};
}
}  // namespace detail

template <Resource T>
ResourceInfo ResourceTraits<T>::makeInfo() {
    return ResourceInfo{.type = TypeTraits<T>::makeTypeInfo()};
}
}  // namespace rtype::ecs
