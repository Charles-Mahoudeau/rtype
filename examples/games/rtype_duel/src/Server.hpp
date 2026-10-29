/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Server
*/

#pragma once

#include <cstdint>

namespace rtype_duel {

/// @brief Runs the headless server until it is stopped.
/// @param port The UDP port the server listens on.
/// @return The exit code of the App.
int runServer(std::uint16_t port);

}  // namespace rtype_duel
