/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentTraits
*/

#pragma once

#include <array>
#include <cstddef>  // offsetof, used by RTYPE_ECS_COMPONENT
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/LayoutHash.hpp"
#include "rtype/ecs/component/NameHash.hpp"
#include "rtype/ecs/detail/ForEach.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/TypeHooks.hpp"

namespace rtype::ecs {
/// @brief One reflected field, known at compile time: the constexpr counterpart of FieldInfo.
struct FieldDescription {
    std::string_view name;             ///< Field name, as written in RTYPE_ECS_COMPONENT.
    FieldType type{FieldType::kBool};  ///< Type of each value.
    std::uint32_t offset{0};           ///< Byte offset inside the component.
    std::uint32_t count{1};            ///< Number of values: 1, or N for an array.
};

/// @brief What RTYPE_ECS_COMPONENT records about a component: its name and its fields.
template <std::size_t N>
struct ComponentDescription {
    std::string_view name;                   ///< Stable component name.
    std::array<FieldDescription, N> fields;  ///< Reflected fields, in declaration order of the macro.
};

namespace detail {
/// @brief Empty type naming a component type, so its describe function can be found by argument-dependent lookup.
template <typename T>
struct ComponentTag {};

/// @brief Describes one field, given its declared type, name and offset.
template <typename Member>
[[nodiscard]] constexpr FieldDescription describeField(std::string_view name, std::size_t offset);

/// @brief Checks that `T` can be a component and gathers its description.
template <typename T, typename... Fields>
[[nodiscard]] constexpr ComponentDescription<sizeof...(Fields)> describeComponent(std::string_view name,
                                                                                  Fields... fields);

/// @brief Builds the lifecycle hooks of `T`, `nullptr` for every operation that is trivial.
template <typename T>
[[nodiscard]] constexpr TypeHooks makeHooks() noexcept;
}  // namespace detail

/// @brief True for a type declared with RTYPE_ECS_COMPONENT.
template <typename T>
concept Component = requires { rtypeEcsDescribe(detail::ComponentTag<T>{}); };

/// @brief Everything known at compile time about a component declared with RTYPE_ECS_COMPONENT.
template <Component T>
struct ComponentTraits {
    /// @brief The name and fields recorded by the macro.
    static constexpr auto kDescription = rtypeEcsDescribe(detail::ComponentTag<T>{});

    /// @brief Stable name, e.g. "rtype.Health".
    static constexpr std::string_view kName = kDescription.name;

    /// @brief hashName() of kName.
    static constexpr std::uint64_t kHash = hashName(kName);

    /// @brief Reflected fields.
    static constexpr auto kFields = kDescription.fields;

    /// @brief Lifecycle hooks, `nullptr` where the operation is trivial.
    static constexpr TypeHooks kHooks = detail::makeHooks<T>();

    /// @brief computeLayoutHash() of makeInfo(), known at compile time: tells whether a registered description is
    /// really this type's.
    static constexpr std::uint64_t kLayoutHash =
        hashLayout(kName, std::is_empty_v<T> ? 0 : sizeof(T), alignof(T), kHooks, kFields);

    /// @return A ComponentInfo with the name, size (0 for an empty struct: a tag), alignment, hooks and fields. The
    /// id, presence, replication and layout hash are left for the registry to set.
    [[nodiscard]] static ComponentInfo makeInfo();
};
}  // namespace rtype::ecs

#include "ComponentTraits.tpp"

/// @brief One field of RTYPE_ECS_COMPONENT: its declared type, its name as a string and its offset.
#define RTYPE_ECS_DETAIL_FIELD(Type, field) \
    ::rtype::ecs::detail::describeField<decltype(Type::field)>(#field, offsetof(Type, field))

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
#define RTYPE_ECS_COMPONENT(Type, Name, ...)                                                             \
    [[maybe_unused]] constexpr auto rtypeEcsDescribe(::rtype::ecs::detail::ComponentTag<Type> /*tag*/) { \
        return ::rtype::ecs::detail::describeComponent<Type>(                                            \
            Name __VA_OPT__(, RTYPE_ECS_DETAIL_FOR_EACH(RTYPE_ECS_DETAIL_FIELD, Type, __VA_ARGS__)));    \
    }                                                                                                    \
    static_assert(true, "RTYPE_ECS_COMPONENT must be followed by a semicolon")
