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

/// @brief Thrown when an input action cannot be declared: a local setup error (a name already taken, an invalid name,
/// a declaration after the layout was frozen).
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
}  // namespace rtype::replication::exceptions
