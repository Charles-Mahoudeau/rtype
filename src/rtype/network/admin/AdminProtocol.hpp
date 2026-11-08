/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AdminProtocol
*/

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

#include "rtype/network/Export.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/network/server/Session.hpp"
#include "rtype/network/simulation/LinkSimulator.hpp"

/// The admin channel: how a tool (the editor) reads and controls a running server. One UDP datagram per request and
/// one per reply, on a port bound to the loopback only, each request carrying the secret token. The wire format is in
/// `docs/network/design.md` (Admin channel).
namespace rtype::network::admin {

/// @brief The version of the admin protocol, given by `Hello`.
inline constexpr std::uint8_t kProtocolVersion = 1;

/// @brief The type byte of a datagram. A reply has the type of its request plus `kReplyFlag`.
///
/// @details The ranges are fixed so that later features need no change here: `0x01` to `0x0F` are the core messages,
/// `0x10` to `0x1F` the network simulation, `0x20` to `0x3F` the world (entities and components). A request in the
/// world range goes to the extension handler of the `AdminServer`; without one it gets `kUnsupported`.
enum class MessageType : std::uint8_t {
    kHello = 0x01,          ///< Checks the token and the version.
    kStatus = 0x02,         ///< Asks for the state of the server.
    kClients = 0x03,        ///< Asks for the connected clients.
    kKick = 0x04,           ///< Disconnects a client.
    kSimulationGet = 0x10,  ///< Asks for the settings of the link simulation.
    kSimulationSet = 0x11,  ///< Sets them.
    kError = 0x7F,          ///< The reply to a request that could not be served (never has `kReplyFlag`).
};

/// @brief Added to the type of a request to make the type of its reply.
inline constexpr std::uint8_t kReplyFlag = 0x80;
/// @brief First type of the network simulation range.
inline constexpr std::uint8_t kSimulationFirst = 0x10;
/// @brief First type of the world range.
inline constexpr std::uint8_t kWorldFirst = 0x20;
/// @brief Last type an extension may use.
inline constexpr std::uint8_t kExtensionLast = 0x3F;

/// @brief Why a request got no result.
enum class ErrorCode : std::uint8_t {
    kBadToken = 1,          ///< The token is not the server's.
    kMalformed = 2,         ///< The body does not decode.
    kUnsupported = 3,       ///< The server does not know this type.
    kUnknownClient = 4,     ///< No such client.
    kUnknownEntity = 5,     ///< No such entity (world range).
    kUnknownComponent = 6,  ///< The entity has no such component, or the world no such type (world range).
    kUnknownField = 7,      ///< The component has no such field, or no such index in it (world range).
    kBadValue = 8,          ///< The value does not read as the field's type, or the field cannot be edited.
};

/// @brief The request that checks the token.
struct HelloRequest {};
/// @brief The request for the state of the server.
struct StatusRequest {};
/// @brief The request for the list of clients.
struct ClientsRequest {};
/// @brief The request that disconnects a client (`DisconnectReason::kKicked`).
struct KickRequest {
    ClientId client{kNoClient};  ///< The client to remove.
};
/// @brief The request for the settings of the link simulation.
struct SimulationGetRequest {};
/// @brief The request that sets the link simulation (latency, jitter, loss on the game socket).
struct SimulationSetRequest {
    simulation::SimulationSettings settings;  ///< Wanted; the server brings them into their ranges.
};
/// @brief A request of the world range, not decoded here.
struct ExtensionRequest {
    std::uint8_t type{0};  ///< The type byte, between `kWorldFirst` and `kExtensionLast`.
    Bytes body;            ///< What follows the header.
};

/// @brief What a client may ask.
using Request = std::variant<HelloRequest, StatusRequest, ClientsRequest, KickRequest, SimulationGetRequest,
                             SimulationSetRequest, ExtensionRequest>;

/// @brief The reply to `Hello`.
struct HelloReply {
    std::uint8_t version{kProtocolVersion};  ///< The protocol version of the server.
};

/// @brief The state of the server.
struct StatusReply {
    std::uint32_t tick{0};               ///< The server tick of the last snapshot.
    std::uint32_t entityCount{0};        ///< Entities with a NetworkId.
    std::uint32_t clientCount{0};        ///< Connected clients.
    std::uint64_t uptimeMs{0};           ///< Since the server opened.
    std::uint64_t datagramsReceived{0};  ///< Game socket counters.
    std::uint64_t datagramsSent{0};
    std::uint64_t bytesReceived{0};
    std::uint64_t bytesSent{0};
};

/// @brief A client of the list.
struct ClientEntry {
    ClientId client{kNoClient};                                ///< Its id.
    server::SessionState state{server::SessionState::kReady};  ///< Welcoming or ready.
    io::Address address;                                       ///< Where it is.
    float rttMs{0.0F};                                         ///< Smoothed round-trip time.
    std::uint64_t droppedMessages{0};                          ///< Unreliable messages dropped.
    std::uint64_t protocolErrors{0};                           ///< Reliable messages out of the window.
};

/// @brief The connected clients. A list too long for one datagram is cut: `total` tells how many there are.
struct ClientsReply {
    std::uint32_t total{0};            ///< How many clients the server has.
    std::vector<ClientEntry> clients;  ///< As many as fit, by id.
};

/// @brief The reply to a kick that found its client.
struct KickReply {
    ClientId client{kNoClient};  ///< The client removed.
};

/// @brief The reply to a request that failed.
struct ErrorReply {
    ErrorCode code{ErrorCode::kMalformed};  ///< Why.
};

/// @brief The settings of the link simulation in force. The reply to both `SimulationGet` and `SimulationSet` (so it
/// always has the type `kSimulationGet` plus `kReplyFlag`): after a set, the values are those the server clamped.
struct SimulationReply {
    simulation::SimulationSettings settings;  ///< In force.
};

/// @brief A reply of the world range, built by the extension handler.
struct ExtensionReply {
    std::uint8_t type{0};  ///< The type of the reply: the request's plus `kReplyFlag`.
    Bytes body;            ///< What follows the header.
};

/// @brief What a server answers.
using Reply =
    std::variant<HelloReply, StatusReply, ClientsReply, KickReply, SimulationReply, ErrorReply, ExtensionReply>;

/// @brief What decodeRequest() found, so a bad body can still be answered.
struct RequestFrame {
    std::uint32_t id{0};                       ///< The id of the request.
    std::string token;                         ///< The secret.
    std::uint8_t type{0};                      ///< The type byte.
    std::optional<Request> request;            ///< The content, nothing when the type or the body is invalid.
    ErrorCode problem{ErrorCode::kMalformed};  ///< Why `request` is empty.
};

/// @brief A reply as it came.
struct DecodedReply {
    std::uint32_t id{0};  ///< The id of the request it answers.
    Reply reply;          ///< What was answered.
};

/// @brief Writes a request datagram.
/// @return The bytes; empty when the request cannot be encoded (a token above 255 bytes, a body too long).
[[nodiscard]] RTYPE_NETWORK_API Bytes encodeRequest(std::uint32_t id, const std::string& token, const Request& request);

/// @brief Reads a request datagram. Never throws, never asserts.
/// @return The frame, or nothing when even the header (type, id, token) is not there: nothing can be answered.
[[nodiscard]] RTYPE_NETWORK_API std::optional<RequestFrame> decodeRequest(std::span<const std::byte> datagram);

/// @brief Writes a reply datagram.
/// @return The bytes; empty when it does not fit in one datagram.
[[nodiscard]] RTYPE_NETWORK_API Bytes encodeReply(std::uint32_t id, const Reply& reply);

/// @brief Reads a reply datagram. Never throws, never asserts.
/// @return The reply, or nothing when it is invalid.
[[nodiscard]] RTYPE_NETWORK_API std::optional<DecodedReply> decodeReply(std::span<const std::byte> datagram);

/// @return How many entries of a clients reply fit in one datagram.
[[nodiscard]] RTYPE_NETWORK_API std::size_t maxClientEntries() noexcept;

}  // namespace rtype::network::admin
