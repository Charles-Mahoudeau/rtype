/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentTraits
*/

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

#include "rtype/ecs/type/FieldTypeOf.hpp"

namespace rtype::ecs {
namespace detail {
template <typename Member>
constexpr FieldDescription describeField(std::string_view name, std::size_t offset) {
    using Element = FieldElement<Member>::Type;
    static_assert(
        HasFieldType<Element>,
        "RTYPE_ECS_COMPONENT: a listed field has a type with no FieldType. Use a supported type (bool, "
        "fixed-size integers, float, double, or a std::array of them) or specialize rtype::ecs::FieldTypeOf.");
    if constexpr (HasFieldType<Element>) {
        return FieldDescription{.name = name,
                                .type = FieldTypeOf<Element>::value,
                                .offset = static_cast<std::uint32_t>(offset),
                                .count = FieldElement<Member>::kCount};
    } else {
        return FieldDescription{};  // Not reached: the static_assert above already failed; avoids a second error.
    }
}

template <typename T, typename... Fields>
constexpr ComponentDescription<sizeof...(Fields)> describeComponent(std::string_view name, Fields... fields) {
    static_assert(std::is_aggregate_v<T>,
                  "RTYPE_ECS_COMPONENT: a component must be a plain struct (no constructors, no private members, "
                  "no virtual functions).");
    static_assert(sizeof...(Fields) == 0 || std::is_standard_layout_v<T>,
                  "RTYPE_ECS_COMPONENT: a component with reflected fields must be standard-layout (all members "
                  "public, no reference members, no virtual base).");
    static_assert(std::is_copy_constructible_v<T>, "RTYPE_ECS_COMPONENT: a component must be copyable.");
    static_assert(std::is_nothrow_move_constructible_v<T>, "RTYPE_ECS_COMPONENT: moving a component must not throw.");
    static_assert(std::is_nothrow_destructible_v<T>, "RTYPE_ECS_COMPONENT: destroying a component must not throw.");
    return ComponentDescription<sizeof...(Fields)>{.name = name, .fields = {fields...}};
}
}  // namespace detail

template <Component T>
ComponentInfo ComponentTraits<T>::makeInfo() {
    ComponentInfo info{.name = std::string{kName},
                       .size = std::is_empty_v<T> ? 0 : sizeof(T),
                       .alignment = alignof(T),
                       .hooks = kHooks};
    info.fields.reserve(kFields.size());
    for (const FieldDescription& field : kFields) {
        info.fields.push_back(FieldInfo{
            .name = std::string{field.name}, .type = field.type, .offset = field.offset, .count = field.count});
    }
    return info;
}
}  // namespace rtype::ecs
