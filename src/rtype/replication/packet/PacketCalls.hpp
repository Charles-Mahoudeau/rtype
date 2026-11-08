/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PacketCalls
*/

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/call/CallTraits.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/replication/Export.hpp"

namespace rtype::replication::packet {
inline constexpr std::string_view kDeclareCall = "rtype.net.declarePacket";        ///< PacketRegistry::declare().
inline constexpr std::string_view kSendToServerCall = "rtype.net.sendToServer";    ///< Client: sends a packet.
inline constexpr std::string_view kSendToClientsCall = "rtype.net.sendToClients";  ///< Server: sends a packet.

/// @brief The arguments of `rtype.net.declarePacket`.
struct DeclareArguments {
    std::string name;       ///< `set_directive`, `game.Buy`...
    std::string direction;  ///< "toServer" or "toClients".
    std::string channel;    ///< "reliable" or "unreliable".
    std::string fields;     ///< One `name:type` or `name:type:maxLength` per line, in wire order.
};
RTYPE_ECS_CALL_DATA(DeclareArguments, "rtype.net.DeclareArguments", name, direction, channel, fields);

/// @brief The arguments of `rtype.net.sendToServer`.
struct SendToServerArguments {
    std::string name;    ///< A declared `toServer` packet.
    std::string values;  ///< The fields as parseValues() reads them.
};
RTYPE_ECS_CALL_DATA(SendToServerArguments, "rtype.net.SendToServerArguments", name, values);

/// @brief The arguments of `rtype.net.sendToClients`.
struct SendToClientsArguments {
    std::string name;         ///< A declared `toClients` packet.
    std::uint32_t client{0};  ///< The client to send it to; 0 for every connected client.
    std::string values;       ///< The fields as parseValues() reads them.
};
RTYPE_ECS_CALL_DATA(SendToClientsArguments, "rtype.net.SendToClientsArguments", name, client, values);

/// @brief The result of both send calls.
struct SendResult {
    bool sent{false};  ///< False when nobody could get it: not connected, or no such client.
};
RTYPE_ECS_CALL_DATA(SendResult, "rtype.net.SendResult", sent);

/// @brief A packet that arrived and passed every check of its schema. The server's scripts get the clients' packets
/// and the client's scripts the server's, in PreUpdate.
struct PacketReceived {
    std::string name;         ///< The packet's name.
    std::uint32_t sender{0};  ///< On the server, the client that sent it; 0 on a client.
    std::string values;       ///< The fields as formatValues() writes them; `engine.net.decode` makes a table.
};
RTYPE_ECS_EVENT(PacketReceived, "net.PacketReceived", name, sender, values);

/// @brief Registers `rtype.net.declarePacket` everywhere, `rtype.net.sendToServer` on the client side and
/// `rtype.net.sendToClients` on the server side, and the Luau forms (`engine.net`). Called by the ReplicationPlugin.
RTYPE_REPLICATION_API void addPacketCalls(ecs::App& app);
}  // namespace rtype::replication::packet
