/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SocketStats
*/

#pragma once

#include <cstdint>

namespace rtype::network::io {
/// @brief Counters of a `UdpSocket`, since its creation.
struct SocketStats {
    std::uint64_t received{0};       ///< Datagrams received and kept.
    std::uint64_t oversized{0};      ///< Datagrams dropped because they were above `kMaxPacketSize`.
    std::uint64_t receiveErrors{0};  ///< Receives that failed and were ignored.
    std::uint64_t sendErrors{0};     ///< Sends that failed and were ignored.
};
}  // namespace rtype::network::io
