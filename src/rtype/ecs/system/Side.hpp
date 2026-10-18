/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Side
*/

#pragma once

#include <cstdint>

namespace rtype::ecs {
/// @brief Where a system runs: the server is authoritative, clients only predict and present.
/// @details In standalone (single-player) mode, server and client systems both run in one world, so the same game
/// code works solo and online. A client system may only write client-only components: anything else would be
/// overwritten by replication.
enum class Side : std::uint8_t {
    kServer,  ///< On the server (and standalone): authoritative gameplay. The default.
    kClient,  ///< On clients (and standalone): input, prediction, presentation.
    kBoth,    ///< Everywhere: what is simulated identically on both sides (bullet motion, for instance).
};
}  // namespace rtype::ecs
