/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RunMode
*/

#pragma once

#include <cstdint>

#include "rtype/ecs/system/Side.hpp"

namespace rtype::ecs {
/// @brief What a process runs: it decides which systems run, from their Side.
enum class RunMode : std::uint8_t {
    kServer,      ///< A dedicated server: server systems and systems of both sides.
    kClient,      ///< A client connected to a server: client systems and systems of both sides.
    kStandalone,  ///< Single player: every system, server and client ones in one world.
};

/// @return Whether a system of this side runs in this mode.
[[nodiscard]] constexpr bool runsIn(Side side, RunMode mode) noexcept {
    switch (mode) {
        case RunMode::kServer:
            return side != Side::kClient;
        case RunMode::kClient:
            return side != Side::kServer;
        case RunMode::kStandalone:
            return true;
    }
    return true;
}
}  // namespace rtype::ecs
