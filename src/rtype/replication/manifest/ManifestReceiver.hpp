/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ManifestReceiver
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <span>

#include "rtype/replication/Export.hpp"
#include "rtype/replication/ReplicationSettings.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/manifest/Manifest.hpp"

namespace rtype::ecs {
class World;
}  // namespace rtype::ecs

namespace rtype::replication::manifest {
/// @brief On the client, collects the manifest the server sent and compares it with the client's own world.
///
/// @details Nothing here throws on bytes from the network: a bad message makes receive() return false.
class RTYPE_REPLICATION_API ManifestReceiver {
  public:
    ManifestReceiver() = default;
    ~ManifestReceiver() = default;
    ManifestReceiver(const ManifestReceiver& other) = default;
    ManifestReceiver& operator=(const ManifestReceiver& other) = default;
    ManifestReceiver(ManifestReceiver&& other) noexcept = default;
    ManifestReceiver& operator=(ManifestReceiver&& other) noexcept = default;

    /// @return The settings of the manifest's end message (the defaults before it arrives).
    [[nodiscard]] const ReplicationSettings& serverSettings() const noexcept;

    /// @return True once a valid end message was received.
    [[nodiscard]] bool isComplete() const noexcept;

    /// @brief Takes one message of the manifest; they arrive in order, on the reliable channel.
    /// @param message The bytes of the message.
    /// @return False when the message is malformed: it does not decode, repeats a short id, an index or a name, comes
    /// after the end, or the end does not match what was received.
    [[nodiscard]] bool receive(std::span<const std::byte> message);

    /// @brief Compares the received manifest with the one built from the client's own world.
    /// @param world The client's world, once every plugin registered its types.
    /// @param inputs The client's input actions.
    /// @return The manifest mapped to the local world when both sides agree, otherwise the first difference. Never
    /// throws.
    [[nodiscard]] ManifestCheck check(const ecs::World& world, const input::InputLayout& inputs) const;

  private:
    std::map<std::uint16_t, ComponentEntry> _components;  ///< Received components by short id.
    std::map<std::uint8_t, input::InputAction> _actions;  ///< Received actions by index.
    ReplicationSettings _settings;                        ///< From the end message.
    std::uint64_t _hash{0};                               ///< From the end message.
    bool _complete{false};                                ///< True after a valid end.
};
}  // namespace rtype::replication::manifest
