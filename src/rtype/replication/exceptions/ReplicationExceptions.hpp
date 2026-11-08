/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ReplicationExceptions
*/

#pragma once

#include <stdexcept>
#include <string>

#include "rtype/replication/Export.hpp"

namespace rtype::replication::exceptions {
/// @brief Thrown when the manifest cannot be built: a local setup error (a replicated type that cannot be sent, too
/// many entries), never caused by the network.
class RTYPE_REPLICATION_API ManifestException : public std::runtime_error {
  public:
    /// @param message What is wrong, naming the entry.
    explicit ManifestException(const std::string& message);
    ~ManifestException() override;
    ManifestException(const ManifestException& other) = default;
    ManifestException& operator=(const ManifestException& other) = default;
    ManifestException(ManifestException&& other) noexcept = default;
    ManifestException& operator=(ManifestException&& other) noexcept = default;
};

/// @brief Thrown when an input action cannot be declared or read: a local setup error (a name already taken, an invalid
/// name, a declaration after the layout was frozen, an unknown action, an entity without a PlayerInput).
class RTYPE_REPLICATION_API InputLayoutException : public std::runtime_error {
  public:
    /// @param message What is wrong, naming the action.
    explicit InputLayoutException(const std::string& message);
    ~InputLayoutException() override;
    InputLayoutException(const InputLayoutException& other) = default;
    InputLayoutException& operator=(const InputLayoutException& other) = default;
    InputLayoutException(InputLayoutException&& other) noexcept = default;
    InputLayoutException& operator=(InputLayoutException&& other) noexcept = default;
};

/// @brief Thrown when the replication cannot start: a local setup error (a rate that does not divide the tick rate, a
/// replicated component registered after the plugin that reads it), never caused by the network.
class RTYPE_REPLICATION_API ReplicationException : public std::runtime_error {
  public:
    /// @param message What is wrong, naming the setting or the component.
    explicit ReplicationException(const std::string& message);
    ~ReplicationException() override;
    ReplicationException(const ReplicationException& other) = default;
    ReplicationException& operator=(const ReplicationException& other) = default;
    ReplicationException(ReplicationException&& other) noexcept = default;
    ReplicationException& operator=(ReplicationException&& other) noexcept = default;
};

/// @brief Thrown when a custom packet cannot be declared or sent: a local setup or scripting error (a name taken, an
/// invalid field, a declaration after the registry was frozen, an unknown packet, a value that breaks the schema, the
/// wrong direction), never caused by the network.
class RTYPE_REPLICATION_API PacketException : public std::runtime_error {
  public:
    /// @param message What is wrong, naming the packet.
    explicit PacketException(const std::string& message);
    ~PacketException() override;
    PacketException(const PacketException& other) = default;
    PacketException& operator=(const PacketException& other) = default;
    PacketException(PacketException&& other) noexcept = default;
    PacketException& operator=(PacketException&& other) noexcept = default;
};
}  // namespace rtype::replication::exceptions
