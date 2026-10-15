/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TypeRegistry
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"

namespace rtype::ecs {
/// @brief What a registry needs to know about one kind of type (components, resources): specialize it next to the
/// kind's description.
///
/// @details A specialization provides:
/// - `Options`: what is chosen when registering a C++ type (storage, presence, replication...);
/// - `kKind`: the TypeKind its C++ types are declared with;
/// - `kName` / `kLabel`: the kind in messages, lowercase and capitalized ("component", "Component");
/// - `validate(info)`: the kind's rules (it calls validateType() itself);
/// - `checkSameOptions(registered, requested)`: the kind's own options, compared when a name is registered again;
/// - `make<T>(options)`: the description of the C++ type `T` with these options.
template <typename Info>
struct KindTraits;

namespace detail {
/// @return An ErrorKind::kNameCollision error: two different names with the same hash.
[[nodiscard]] RTYPE_ECS_API Error nameCollision(std::string_view label, std::string_view registered,
                                                std::string_view requested);

/// @return An ErrorKind::kLayoutMismatch error: the name is registered with another layout.
[[nodiscard]] RTYPE_ECS_API Error layoutMismatch(std::string_view label, std::string_view name);

/// @return An ErrorKind::kOptionsMismatch error naming the option and both values.
[[nodiscard]] RTYPE_ECS_API Error optionsMismatch(std::string_view label, std::string_view name,
                                                  std::string_view option, std::string_view registered,
                                                  std::string_view requested);

/// @return An ErrorKind::kInvalidType error: every id is taken.
[[nodiscard]] RTYPE_ECS_API Error noIdLeft(std::string_view label, std::string_view name);

/// @brief Compares what every kind shares when a name is registered again: the name itself, the layout, the
/// presence and the replication.
[[nodiscard]] RTYPE_ECS_API Result<void> checkSameType(std::string_view label, const TypeInfo& registered,
                                                       const TypeInfo& requested);
}  // namespace detail

/// @brief True for a C++ type declared with the macro of the kind whose description is `Info`.
/// @details A named concept, so the declaration and the out-of-line definition of a constrained member spell the
/// constraint the same way (MSVC requires it).
template <typename T, typename Info>
concept OfKind = Described<T> && TypeTraits<T>::kKind == KindTraits<Info>::kKind;

/// @brief The catalog of the types of one kind (components, resources) in one world.
///
/// @details Registering validates a description, computes its layout hash and gives it a dense TypeId, so ids can
/// index plain vectors. Types are then found by id, by name or by name hash. A name identifies one type: registering
/// the same type again returns its id, while the same name with another layout or other options is refused.
/// Registered descriptions never change, and pointers returned by the lookups stay valid for the registry's lifetime
/// (a deque keeps their addresses, also when the registry is moved).
/// @tparam Info The kind's description, which holds a TypeInfo named `type`; KindTraits<Info> must be specialized.
template <typename Info>
class TypeRegistry {
  public:
    /// @brief The kind's specifics.
    using Traits = KindTraits<Info>;

    /// @brief What is chosen when registering a C++ type of this kind.
    using Options = Traits::Options;

    TypeRegistry() = default;
    ~TypeRegistry() = default;
    TypeRegistry(const TypeRegistry& other) = delete;
    TypeRegistry& operator=(const TypeRegistry& other) = delete;
    TypeRegistry(TypeRegistry&& other) noexcept = default;
    TypeRegistry& operator=(TypeRegistry&& other) noexcept = default;

    /// @return The number of registered types; ids go from 0 to this count minus one.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @return The ids of the types the server sends to clients, in registration order.
    [[nodiscard]] std::span<const TypeId> getReplicatedIds() const noexcept;

    /// @brief Registers a type described at run time (the path scripts will use). Never throws on a refusal.
    /// @param info The description; its id and layout hash are ignored and set by the registry.
    /// @return The type's id (its existing id if this exact type is already registered), or an error:
    /// ErrorKind::kInvalidType, kLayoutMismatch, kOptionsMismatch or kNameCollision.
    [[nodiscard]] Result<TypeId> registerType(Info info);

    /// @brief Registers a C++ type declared with this kind's macro.
    /// @return The type's id (its existing id if this exact type is already registered).
    /// @throws exceptions::TypeRegistrationException If the type cannot be registered.
    template <OfKind<Info> T>
    TypeId registerType(Options options = {});

    /// @return The type with this id, or nullptr.
    [[nodiscard]] const Info* findById(TypeId id) const noexcept;

    /// @return The type with this name, or nullptr.
    [[nodiscard]] const Info* findByName(std::string_view name) const noexcept;

    /// @return The type whose name has this hash (see hashName()), or nullptr.
    [[nodiscard]] const Info* findByHash(std::uint64_t nameHash) const noexcept;

    /// @return The C++ type `T`, or nullptr if it is not registered. Uses its compile-time name and layout hashes: a
    /// type registered under `T`'s name with another layout (a run-time description that only shares the name) is
    /// not `T`, so this returns nullptr, and registering `T` then fails with kLayoutMismatch.
    template <OfKind<Info> T>
    [[nodiscard]] const Info* find() const noexcept;

  private:
    std::deque<Info> _infos;                               ///< Types by id; a deque keeps their addresses.
    std::unordered_map<std::uint64_t, TypeId> _idsByHash;  ///< Ids by name hash.
    std::vector<TypeId> _replicatedIds;                    ///< Ids of the replicated types.
};
}  // namespace rtype::ecs

#include "TypeRegistry.tpp"
