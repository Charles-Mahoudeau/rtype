/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AdminSettings
*/

#pragma once

#include <cstdint>
#include <string>

namespace rtype::network::admin {
/// @brief Turns the admin channel of a server on: a tool (the editor) controls the server through it.
struct AdminSettings {
    /// @brief Port of the channel, on `127.0.0.1`; 0 picks a free one (see `AdminServer::localPort()`).
    std::uint16_t port{0};
    /// @brief The secret every admin request carries; it must not be empty.
    std::string token;
};
}  // namespace rtype::network::admin
