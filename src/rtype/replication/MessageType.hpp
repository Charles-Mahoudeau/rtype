/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MessageType
*/

#pragma once

#include <cstdint>

namespace rtype::replication {
/// @brief The first byte of a message the replication sends through the network library. Values below 0x20 belong
/// to the transport.
enum class MessageType : std::uint8_t {
    kManifestComponent = 0x20,    ///< S → C: a replicated component of the manifest.
    kManifestPacket = 0x21,       ///< S → C: a custom packet of the manifest.
    kManifestPrefab = 0x22,       ///< S → C: a prefab of the manifest.
    kManifestInputAction = 0x23,  ///< S → C: an input action of the manifest.
    kManifestEnd = 0x24,          ///< S → C: the end of the manifest, with the server's settings.
    kSnapshot = 0x30,             ///< S → C: the state of the world.
    kInput = 0x31,                ///< C → S: the last inputs of the player.
    kCustomPacket = 0x32,         ///< Both ways: a game-defined packet.
};
}  // namespace rtype::replication
