/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EntityMapper
*/

#pragma once

#include <cstdint>

#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::replication::serialization {
/// @brief Translates entity references between the local world and the network.
///
/// @details An Entity holds a slot index local to one world, so the field serializer never sends it: it asks the
/// mapper for the entity's NetworkId instead, and back for the local entity of a NetworkId. The server maps with its
/// tracking table, the client with its `NetworkId → Entity` map.
class EntityMapper {
  public:
    EntityMapper() = default;
    virtual ~EntityMapper() = default;
    EntityMapper(const EntityMapper&) = delete;
    EntityMapper& operator=(const EntityMapper&) = delete;
    EntityMapper(EntityMapper&&) = delete;
    EntityMapper& operator=(EntityMapper&&) = delete;

    /// @brief Gives the NetworkId of a local entity.
    /// @param entity The local entity.
    /// @return The NetworkId of the entity, 0 if it is null or not replicated.
    [[nodiscard]] virtual std::uint32_t toNetwork(ecs::Entity entity) const noexcept = 0;

    /// @brief Gives the local entity of a NetworkId.
    /// @param networkId The NetworkId received.
    /// @return The local entity of a NetworkId, the null entity if unknown or 0.
    [[nodiscard]] virtual ecs::Entity toLocal(std::uint32_t networkId) const noexcept = 0;
};
}  // namespace rtype::replication::serialization
