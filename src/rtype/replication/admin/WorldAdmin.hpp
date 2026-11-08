/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldAdmin
*/

#pragma once

#include <optional>

#include "rtype/network/admin/AdminProtocol.hpp"
#include "rtype/network/admin/WorldProtocol.hpp"
#include "rtype/replication/Export.hpp"

namespace rtype::ecs {
class Commands;
class Schedules;
class World;
}  // namespace rtype::ecs

/// The server side of the world messages of the admin channel (`network/admin/WorldProtocol.hpp`): it reads the
/// entities and their components of the server's world, and edits them for a tool such as the editor.
namespace rtype::replication::admin {

/// @brief Answers the world requests of the admin channel.
///
/// @details Reads are answered at once from the world. A change (set a value, spawn, despawn) is recorded in the
/// `Commands` bound by bind() and applied when the system that polls the channel ends its phase, so it never happens
/// in the middle of a system: with no commands bound, a change is refused (`kUnsupported`). Pausing, resuming and
/// stepping go through the `Schedules` given at construction; without them they are refused too. Requests are
/// untrusted: an entity that is not alive, an unknown name or a value that does not read is an error reply, never a
/// crash. The server stays authoritative: a tool edits the server's world, nothing flows back from a client.
class RTYPE_REPLICATION_API WorldAdmin {
  public:
    /// @param world The world to read. Must outlive this object.
    /// @param schedules The schedules to pause and step; may be null. Must outlive this object.
    explicit WorldAdmin(const ecs::World& world, ecs::Schedules* schedules = nullptr) noexcept;

    /// @brief Gives the commands that changes are recorded in, for the length of one poll; null takes them back.
    void bind(ecs::Commands* commands) noexcept;

    /// @brief Answers a request of the world range.
    /// @return The reply, or nothing when the request is not a world request (the caller answers `kUnsupported`).
    [[nodiscard]] std::optional<network::admin::Reply> handle(const network::admin::ExtensionRequest& request);

  private:
    [[nodiscard]] network::admin::Reply entities(const network::admin::EntitiesRequest& request) const;
    [[nodiscard]] network::admin::Reply inspect(const network::admin::InspectRequest& request) const;
    [[nodiscard]] network::admin::Reply setField(const network::admin::SetFieldRequest& request);
    [[nodiscard]] network::admin::Reply spawn(const network::admin::SpawnRequest& request);
    [[nodiscard]] network::admin::Reply despawn(const network::admin::DespawnRequest& request);
    [[nodiscard]] network::admin::Reply run(const network::admin::RunRequest& request);

    const ecs::World* _world;           ///< Read.
    ecs::Schedules* _schedules;         ///< Paused and stepped; may be null.
    ecs::Commands* _commands{nullptr};  ///< Where changes are recorded; null outside a poll.
};
}  // namespace rtype::replication::admin
