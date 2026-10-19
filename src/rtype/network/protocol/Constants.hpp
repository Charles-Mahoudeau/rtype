/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Constants
*/

#pragma once

#include <cstddef>
#include <cstdint>

namespace rtype::network::protocol {

/// @brief Identifies R-Type datagrams ("RTYP"); anything else is dropped.
inline constexpr std::uint32_t kProtocolId = 0x52545950;

/// @brief Bumped by every change of a wire layout or of a constant below.
inline constexpr std::uint16_t kProtocolVersion = 1;

/// @brief Largest datagram, in bytes, that fits a typical path MTU.
inline constexpr std::size_t kMaxPacketSize = 1200;

/// @brief Largest message payload, in bytes, that fits a datagram with its headers.
inline constexpr std::size_t kMaxMessageSize = 1000;

/// @brief Padding added to handshake datagrams so a reply is never larger than the request.
inline constexpr std::size_t kHandshakePadding = 64;

/// @brief Number of reliable messages that may be in flight at once.
inline constexpr std::size_t kReliableWindow = 256;

/// @brief Number of reliable messages that may wait to be sent before the connection overflows.
inline constexpr std::size_t kMaxPendingReliable = 1024;

/// @brief Number of datagrams a connection may send in one flush.
inline constexpr std::size_t kMaxDatagramsPerFlush = 8;

/// @brief Number of sent datagrams remembered to match their acknowledgements.
inline constexpr std::size_t kSentPacketHistory = 1024;

}  // namespace rtype::network::protocol
