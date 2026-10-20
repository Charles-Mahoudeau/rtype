/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ReplicationExceptions
*/

#include "rtype/replication/exceptions/ReplicationExceptions.hpp"

#include <stdexcept>
#include <string>

namespace rtype::replication::exceptions {
ManifestException::ManifestException(const std::string& message) : std::runtime_error{message} {}

ManifestException::~ManifestException() = default;

InputLayoutException::InputLayoutException(const std::string& message) : std::runtime_error{message} {}

InputLayoutException::~InputLayoutException() = default;

ReplicationException::ReplicationException(const std::string& message) : std::runtime_error{message} {}

ReplicationException::~ReplicationException() = default;
}  // namespace rtype::replication::exceptions
