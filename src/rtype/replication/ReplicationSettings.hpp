/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ReplicationSettings
*/

#pragma once

#include <cstdint>

namespace rtype::replication {
/// @brief The settings of the replication, with the design's defaults. The server sends the ones a client needs
/// at the end of the manifest.
struct ReplicationSettings {
    std::uint8_t tickRate{60};                   ///< Simulation ticks per second (Hz).
    std::uint8_t snapshotRate{60};               ///< Snapshots per second (Hz); divides the tick rate.
    std::uint16_t reconnectionGraceSeconds{60};  ///< How long a lost session waits for its client; 0 disables it.

    friend bool operator==(const ReplicationSettings& left, const ReplicationSettings& right) noexcept = default;
};
}  // namespace rtype::replication
