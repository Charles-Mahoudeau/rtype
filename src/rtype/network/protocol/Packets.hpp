/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Packets
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <variant>

#include "rtype/network/Types.hpp"
#include "rtype/network/bytes/Codec.hpp"
#include "rtype/network/protocol/Constants.hpp"

namespace rtype::network::protocol {
/// @brief The first byte of a datagram: which transport packet follows. 0 is never valid.
enum class PacketKind : std::uint8_t {
    kConnectRequest = 0x01,
    kChallenge = 0x02,
    kChallengeResponse = 0x03,
    kAccepted = 0x04,
    kRejected = 0x05,
    kData = 0x06,
    kDisconnect = 0x07,
};

/// @brief Tells whether `kind` is a value of the wire (1 to 7, never 0).
[[nodiscard]] constexpr bool isValidWireValue(const PacketKind kind) noexcept {
    return kind >= PacketKind::kConnectRequest && kind <= PacketKind::kDisconnect;
}

/// @brief Client to server: "I want to connect". Padded to `kHandshakePadding` so that no answer is bigger.
struct ConnectRequest {
    static constexpr auto kKind = PacketKind::kConnectRequest;  ///< The kind byte.
    std::uint16_t version{0};                                   ///< The protocol version of the client.
    ClientId resumeClient{kNoClient};                           ///< The client to resume, or `kNoClient`.
    std::uint64_t resumeToken{0};                               ///< Proves the right to resume.

    /// @brief Lists the fields in wire order; a wrong protocol id makes the stream invalid.
    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.constant(kProtocolId);
        stream.value(self.version);
        stream.value(self.resumeClient);
        stream.value(self.resumeToken);
        stream.padTo(kHandshakePadding);
    }
};

/// @brief Server to client: proves the client owns its address, since only the real address receives it.
struct Challenge {
    static constexpr auto kKind = PacketKind::kChallenge;  ///< The kind byte.
    std::uint64_t challenge{0};                            ///< The random value to echo.

    /// @brief Lists the fields in wire order.
    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.challenge);
    }
};

/// @brief Client to server: echoes the challenge, padded to `kHandshakePadding`.
struct ChallengeResponse {
    static constexpr auto kKind = PacketKind::kChallengeResponse;  ///< The kind byte.
    std::uint64_t challenge{0};                                    ///< The echoed value.

    /// @brief Lists the fields in wire order.
    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.challenge);
        stream.padTo(kHandshakePadding);
    }
};

/// @brief Server to client: the session exists; its token must be in every later packet.
struct Accepted {
    static constexpr auto kKind = PacketKind::kAccepted;  ///< The kind byte.
    std::uint32_t sessionToken{0};                        ///< The token of every later packet.
    ClientId client{kNoClient};                           ///< The id given to the client.
    std::uint64_t resumeToken{0};                         ///< The token to resume the session later.

    /// @brief Lists the fields in wire order.
    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.sessionToken);
        stream.value(self.client);
        stream.value(self.resumeToken);
    }
};

/// @brief Server to client: why the connection is refused.
struct Rejected {
    static constexpr auto kKind = PacketKind::kRejected;  ///< The kind byte.
    RejectReason reason{RejectReason::kVersionMismatch};  ///< Why.

    /// @brief Lists the fields in wire order.
    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.reason);
    }
};

/// @brief Both ways: everything once connected. `messages` points into the datagram, so it is only valid while the
/// received buffer lives.
struct Data {
    static constexpr auto kKind = PacketKind::kData;  ///< The kind byte.
    std::uint32_t sessionToken{0};                    ///< The token given by `Accepted`.
    std::uint16_t sequence{0};                        ///< The sequence number of this datagram.
    std::uint16_t ack{0};                             ///< The latest sequence received from the peer.
    std::uint32_t ackBits{0};                         ///< The 32 sequences before `ack`, one bit each.
    std::span<const std::byte> messages;              ///< The messages, up to the end of the datagram.

    /// @brief Lists the fields in wire order.
    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.sessionToken);
        stream.value(self.sequence);
        stream.value(self.ack);
        stream.value(self.ackBits);
        stream.rest(self.messages);
    }
};

/// @brief Both ways: closes the connection.
struct Disconnect {
    static constexpr auto kKind = PacketKind::kDisconnect;  ///< The kind byte.
    std::uint32_t sessionToken{0};                          ///< The token of the session.
    DisconnectReason reason{DisconnectReason::kGraceful};   ///< Why.

    /// @brief Lists the fields in wire order.
    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.sessionToken);
        stream.value(self.reason);
    }
};

/// @brief Any transport packet.
using Packet = std::variant<ConnectRequest, Challenge, ChallengeResponse, Accepted, Rejected, Data, Disconnect>;

/// @brief The codec of the transport packets.
using PacketCodec = bytes::Codec<PacketKind, Packet>;
}  // namespace rtype::network::protocol
