/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NetworkExceptions
*/

#include "rtype/network/exceptions/NetworkExceptions.hpp"

#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>

namespace rtype::network::exceptions {

SocketException::SocketException(const std::uint16_t port, const std::string& reason)
    : std::runtime_error{std::format("Cannot bind UDP port {}: {}", port, reason)}, _port{port} {}

std::uint16_t SocketException::getPort() const noexcept { return _port; }

}  // namespace rtype::network::exceptions
