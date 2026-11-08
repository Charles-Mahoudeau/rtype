/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** LocalSession
*/

#pragma once

#include <utility>
#include <vector>

#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/replication/Export.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/packet/PacketSchema.hpp"

namespace rtype::replication::standalone {
/// @brief A packet sent in a standalone world, waiting for the next PreUpdate to be delivered.
struct QueuedPacket {
    packet::PacketDirection direction{packet::PacketDirection::kToServer};  ///< Which side it travels toward.
    network::Bytes message;                                                 ///< The encoded packet.
};

/// @brief The state of the one local client of a standalone world: its input and the packets in flight.
///
/// @details A standalone world is a server with one client, client 1, and no socket. The input the player gives is
/// kept here until the FixedUpdate hands it to the entity that client owns; the packets the two sides send each other
/// wait here until the next PreUpdate. Inserted by the StandalonePlugin.
class RTYPE_REPLICATION_API LocalSession {
  public:
    static constexpr network::ClientId kClient = 1;  ///< The id of the local client.

    LocalSession() = default;
    ~LocalSession() = default;
    LocalSession(const LocalSession& other) = default;
    LocalSession& operator=(const LocalSession& other) = default;
    LocalSession(LocalSession&& other) noexcept = default;
    LocalSession& operator=(LocalSession&& other) noexcept = default;

    /// @brief Sets the input the local player gives now; the game calls it every frame.
    void setInput(const component::PlayerInput& input) { _input = input; }

    /// @return The input the local player gives now; neutral before any.
    [[nodiscard]] const component::PlayerInput& input() const noexcept { return _input; }

    /// @brief Queues a packet for the next PreUpdate.
    void queuePacket(const packet::PacketDirection direction, network::Bytes message) {
        _packets.push_back(QueuedPacket{.direction = direction, .message = std::move(message)});
    }

    /// @return The queued packets, which are removed.
    [[nodiscard]] std::vector<QueuedPacket> takePackets() { return std::exchange(_packets, {}); }

    /// @return True the first time, when the client has not been announced yet; false after.
    [[nodiscard]] bool takeConnect() noexcept { return !std::exchange(_announced, true); }

  private:
    component::PlayerInput _input;       ///< The input of the player now.
    std::vector<QueuedPacket> _packets;  ///< Sent, not delivered yet.
    bool _announced{false};              ///< Whether net.ClientConnected was raised.
};

RTYPE_ECS_RESOURCE(LocalSession, "net.LocalSession");
}  // namespace rtype::replication::standalone
