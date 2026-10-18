/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Types
*/

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace rtype::network {

/// @brief Identifies a connected player: given by the server from 1 upwards and never reused.
using ClientId = std::uint32_t;

/// @brief The id that means "no client".
inline constexpr ClientId kNoClient = 0;

/// @brief The clock of the network: monotonic, it never jumps with the wall clock.
using Clock = std::chrono::steady_clock;

/// @brief A point in time of `Clock`; classes receive it instead of reading the clock.
using TimePoint = Clock::time_point;

/// @brief An owned buffer of bytes.
using Bytes = std::vector<std::byte>;

/// @brief The delivery guarantee of a message.
enum class Channel : std::uint8_t {
    kUnreliable = 0,
    kReliable = 1,
};

/// @brief Why a connection ended. Wire values: 0 is never valid.
enum class DisconnectReason : std::uint8_t {
    kGraceful = 1,
    kTimeout = 2,
    kKicked = 3,
    kProtocolError = 4,
    kManifestMismatch = 5,
    kOverflow = 6,
    kServerShutdown = 7,
};

/// @brief Why a server refused a connection request. Wire values: 0 is never valid.
enum class RejectReason : std::uint8_t {
    kVersionMismatch = 1,
    kServerFull = 2,
    kSessionExpired = 3,
};

}  // namespace rtype::network
