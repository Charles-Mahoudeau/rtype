/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NetworkExceptions
*/

#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include "rtype/network/Export.hpp"

namespace rtype::network::exceptions {
/// @brief Thrown when a UDP socket cannot be opened or bound: a local setup error, never caused by the network.
class RTYPE_NETWORK_API SocketException : public std::runtime_error {
  public:
    /// @param port The port that was asked for.
    /// @param reason The message of the system.
    SocketException(std::uint16_t port, const std::string& reason);

    /// @brief Destroys the exception.
    ~SocketException() override = default;

    /// @brief Copies the exception.
    SocketException(const SocketException&) = default;

    /// @brief Copy-assigns the exception.
    SocketException& operator=(const SocketException&) = default;

    /// @brief Moves the exception.
    SocketException(SocketException&&) = default;

    /// @brief Move-assigns the exception.
    SocketException& operator=(SocketException&&) = default;

    /// @brief Gives the port that was asked for.
    [[nodiscard]] std::uint16_t getPort() const noexcept;

  private:
    std::uint16_t _port;  ///< The port that was asked for.
};
}  // namespace rtype::network::exceptions
