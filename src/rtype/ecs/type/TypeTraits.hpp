/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TypeTraits
*/

#pragma once

#include <array>
#include <cstddef>  // offsetof, used by RTYPE_ECS_DETAIL_FIELD
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "core/hash/NameHash.hpp"
#include "rtype/ecs/detail/ForEach.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/LayoutHash.hpp"
#include "rtype/ecs/type/TypeHooks.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::ecs {
/// @brief Which kind of ECS data a C++ type was declared as.
enum class TypeKind : std::uint8_t {
    kComponent,  ///< Declared with RTYPE_ECS_COMPONENT: one value per entity that has it.
    kResource,   ///< Declared with RTYPE_ECS_RESOURCE: one value per world.
    kEvent,      ///< Declared with RTYPE_ECS_EVENT: a message sent between systems, kept for two updates.
};

/// @brief One reflected field, known at compile time: the constexpr counterpart of FieldInfo.
struct FieldDescription {
    std::string_view name;             ///< Field name, as written in the declaring macro.
    FieldType type{FieldType::kBool};  ///< Type of each value.
    std::uint32_t offset{0};           ///< Byte offset inside the value.
    std::uint32_t count{1};            ///< Number of values: 1, or N for an array.
};

/// @brief What a declaring macro records about a type: its name, its kind and its fields.
template <std::size_t N>
struct TypeDescription {
    std::string_view name;                   ///< Stable name.
    TypeKind kind{TypeKind::kComponent};     ///< Which macro declared it.
    std::array<FieldDescription, N> fields;  ///< Reflected fields, in declaration order of the macro.
};

namespace detail {
/// @brief Empty type naming a C++ type, so its describe function can be found by argument-dependent lookup.
template <typename T>
struct TypeTag {};

/// @brief Describes one field, given its declared type, name and offset.
template <typename Member>
[[nodiscard]] constexpr FieldDescription describeField(std::string_view name, std::size_t offset);
}  // namespace detail

/// @brief True for a type declared with one of the declaring macros (RTYPE_ECS_COMPONENT, ...).
template <typename T>
concept Described = requires { rtypeEcsDescribe(detail::TypeTag<T>{}); };

/// @brief Everything known at compile time about a declared type, whatever its kind.
template <Described T>
struct TypeTraits {
    /// @brief The name, kind and fields recorded by the macro.
    static constexpr auto kDescription = rtypeEcsDescribe(detail::TypeTag<T>{});

    /// @brief Stable name, e.g. "rtype.Health".
    static constexpr std::string_view kName = kDescription.name;

    /// @brief Which macro declared it.
    static constexpr TypeKind kKind = kDescription.kind;

    /// @brief hashName() of kName.
    static constexpr std::uint64_t kHash = core::hash::hashName(kName);

    /// @brief Reflected fields.
    static constexpr auto kFields = kDescription.fields;

    /// @brief Bytes per value: 0 for an empty type (a tag).
    static constexpr std::size_t kSize = std::is_empty_v<T> ? 0 : sizeof(T);

    /// @brief Required alignment of a value.
    static constexpr std::size_t kAlignment = alignof(T);

    /// @brief Whether a value can be copied.
    static constexpr bool kCopyable = std::is_copy_constructible_v<T>;

    /// @brief Lifecycle hooks, `nullptr` where the operation is trivial or unsupported.
    static constexpr TypeHooks kHooks = detail::makeHooks<T>();

    /// @brief computeLayoutHash() of makeTypeInfo(), known at compile time: tells whether a registered description is
    /// really this type's.
    static constexpr std::uint64_t kLayoutHash = hashLayout(kName, kSize, kAlignment, kHooks, kCopyable, kFields);

    /// @return A TypeInfo with the name, size, alignment, hooks, copyability and fields. The id, presence,
    /// replication and layout hash are left for the registry to set.
    [[nodiscard]] static TypeInfo makeTypeInfo();
};
}  // namespace rtype::ecs

#include "TypeTraits.tpp"

/// @brief One field of a declaring macro: its declared type, its name as a string and its offset.
#define RTYPE_ECS_DETAIL_FIELD(Type, field) \
    ::rtype::ecs::detail::describeField<decltype(Type::field)>(#field, offsetof(Type, field))

/// @brief The body shared by the declaring macros: defines the describe function of `Type`, found by
/// argument-dependent lookup, which checks `Type` with `Describe` and records its name, kind and fields.
#define RTYPE_ECS_DETAIL_DESCRIBE(Describe, Type, Name, ...)                                          \
    [[maybe_unused]] constexpr auto rtypeEcsDescribe(::rtype::ecs::detail::TypeTag<Type> /*tag*/) {   \
        return Describe<Type>(                                                                        \
            Name __VA_OPT__(, RTYPE_ECS_DETAIL_FOR_EACH(RTYPE_ECS_DETAIL_FIELD, Type, __VA_ARGS__))); \
    }
