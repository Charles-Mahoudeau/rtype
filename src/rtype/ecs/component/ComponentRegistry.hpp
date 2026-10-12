/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentRegistry
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
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/error/Result.hpp"

namespace rtype::ecs {
/// @brief How a C++ component is stored and sent over the network, chosen when it is registered.
struct ComponentOptions {
    StorageKind storage{StorageKind::kSparseSet};         ///< Where the values live.
    Presence presence{Presence::kBoth};                   ///< On which side of the network it exists.
    ReplicationMode replication{ReplicationMode::kNone};  ///< How the server sends it.
};

/// @brief The catalog of the component types of one world.
///
/// @details Registering validates a ComponentInfo, computes its layout hash and gives it a dense ComponentId, so ids
/// can index plain vectors. Components are then found by id, by name or by name hash. A name identifies one
/// component: registering the same component again returns its id, while the same name with another layout or
/// other options is refused. Registered components never change, and pointers returned by the lookups stay valid
/// for the registry's lifetime.
/// @code
/// rtype::ecs::ComponentRegistry registry;
/// const rtype::ecs::ComponentId transform =
///     registry.registerComponent<Transform>({.replication = rtype::ecs::ReplicationMode::kEveryChange});
/// const rtype::ecs::ComponentInfo* info = registry.findByName("rtype.Transform");
/// @endcode
class RTYPE_ECS_API ComponentRegistry {
  public:
    ComponentRegistry() = default;
    ~ComponentRegistry() = default;
    ComponentRegistry(const ComponentRegistry& other) = delete;
    ComponentRegistry& operator=(const ComponentRegistry& other) = delete;
    ComponentRegistry(ComponentRegistry&& other) noexcept = default;
    ComponentRegistry& operator=(ComponentRegistry&& other) noexcept = default;

    /// @return The number of registered components; ids go from 0 to this count minus one.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @return The ids of the components the server sends to clients, in registration order.
    [[nodiscard]] std::span<const ComponentId> getReplicatedIds() const noexcept;

    /// @brief Registers a component described at run time (the path scripts will use). Never throws on a refusal.
    /// @param info The description; its id and layout hash are ignored and set by the registry.
    /// @return The component's id (its existing id if this exact component is already registered), or an error:
    /// ErrorKind::kInvalidComponent, kLayoutMismatch, kOptionsMismatch or kNameCollision.
    [[nodiscard]] Result<ComponentId> registerComponent(ComponentInfo info);

    /// @brief Registers a C++ component declared with RTYPE_ECS_COMPONENT.
    /// @return The component's id (its existing id if this exact component is already registered).
    /// @throws exceptions::ComponentRegistrationException If the component cannot be registered.
    template <Component T>
    ComponentId registerComponent(ComponentOptions options = {});

    /// @return The component with this id, or nullptr.
    [[nodiscard]] const ComponentInfo* findById(ComponentId id) const noexcept;

    /// @return The component with this name, or nullptr.
    [[nodiscard]] const ComponentInfo* findByName(std::string_view name) const noexcept;

    /// @return The component whose name has this hash (see hashName()), or nullptr.
    [[nodiscard]] const ComponentInfo* findByHash(std::uint64_t nameHash) const noexcept;

    /// @return The C++ component `T`, or nullptr if it is not registered. Uses its compile-time name hash.
    template <Component T>
    [[nodiscard]] const ComponentInfo* find() const noexcept;

  private:
    std::deque<ComponentInfo> _infos;                           ///< Components by id; a deque keeps their addresses.
    std::unordered_map<std::uint64_t, ComponentId> _idsByHash;  ///< Ids by name hash.
    std::vector<ComponentId> _replicatedIds;                    ///< Ids of the replicated components.
};
}  // namespace rtype::ecs

#include "ComponentRegistry.tpp"
