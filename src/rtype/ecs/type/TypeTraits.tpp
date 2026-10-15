/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TypeTraits
*/

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "rtype/ecs/type/FieldTypeOf.hpp"

namespace rtype::ecs {
namespace detail {
template <typename Member>
constexpr FieldDescription describeField(std::string_view name, std::size_t offset) {
    using Element = FieldElement<Member>::Type;
    static_assert(
        HasFieldType<Element>,
        "RTYPE_ECS: a listed field has a type with no FieldType. Use a supported type (bool, fixed-size integers, "
        "float, double, Entity, or a std::array of them) or specialize rtype::ecs::FieldTypeOf.");
    if constexpr (HasFieldType<Element>) {
        return FieldDescription{.name = name,
                                .type = FieldTypeOf<Element>::value,
                                .offset = static_cast<std::uint32_t>(offset),
                                .count = FieldElement<Member>::kCount};
    } else {
        return FieldDescription{};  // Not reached: the static_assert above already failed; avoids a second error.
    }
}
}  // namespace detail

template <Described T>
TypeInfo TypeTraits<T>::makeTypeInfo() {
    TypeInfo info{
        .name = std::string{kName}, .size = kSize, .alignment = kAlignment, .hooks = kHooks, .copyable = kCopyable};
    info.fields.reserve(kFields.size());
    for (const FieldDescription& field : kFields) {
        info.fields.push_back(FieldInfo{
            .name = std::string{field.name}, .type = field.type, .offset = field.offset, .count = field.count});
    }
    return info;
}
}  // namespace rtype::ecs
