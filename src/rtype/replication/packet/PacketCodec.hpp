/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PacketCodec
*/

#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "rtype/network/Types.hpp"
#include "rtype/replication/Export.hpp"
#include "rtype/replication/packet/PacketSchema.hpp"

namespace rtype::replication::packet {
/// @brief A packet read from the network.
struct DecodedPacket {
    std::string name;     ///< The packet's name.
    PacketValues values;  ///< Checked against its schema.
};

/// @brief Reads values from the text a script gives: for each field in order, `<size>:<bytes>`. A number is its decimal
/// text, a bool is `1` or `0`.
/// @param schema The packet.
/// @param text The text.
/// @param error Set to what is wrong when the result is empty.
/// @return The values, or nothing when a field is missing, an integer is out of range or not whole, a float is not
/// finite, a string is too long, or bytes remain.
[[nodiscard]] RTYPE_REPLICATION_API std::optional<PacketValues> parseValues(const PacketSchema& schema,
                                                                            std::string_view text, std::string& error);

/// @brief Writes values the way parseValues() reads them: what a script gets from a received packet.
[[nodiscard]] RTYPE_REPLICATION_API std::string formatValues(const PacketSchema& schema, const PacketValues& values);

/// @brief Writes a message: type byte, name, schema hash, then each field.
/// @param schema The packet.
/// @param values Checked values (parseValues()).
/// @return The message, ready for the transport.
[[nodiscard]] RTYPE_REPLICATION_API network::Bytes encodePacket(const PacketSchema& schema, const PacketValues& values);

/// @brief Reads a kCustomPacket message from the network; never throws.
/// @param registry The declared packets.
/// @param direction The only direction accepted: what the receiver can get.
/// @param payload The message, type byte included.
/// @return The packet, or nothing when it is malformed, unknown, in the wrong direction, from another declaration (its
/// hash differs), or has a value that breaks its schema.
[[nodiscard]] RTYPE_REPLICATION_API std::optional<DecodedPacket> decodePacket(const PacketRegistry& registry,
                                                                              PacketDirection direction,
                                                                              std::span<const std::byte> payload);

/// @brief Tells whether decodePacket() accepts a message that came on a channel: the channel must be the one the
/// packet's schema declares.
/// @param channel The channel the message arrived on.
[[nodiscard]] RTYPE_REPLICATION_API bool isAcceptedPacket(const PacketRegistry& registry, PacketDirection direction,
                                                          network::Channel channel, std::span<const std::byte> payload);
}  // namespace rtype::replication::packet
