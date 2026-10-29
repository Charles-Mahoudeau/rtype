/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Client
*/

#pragma once

#include <cstdint>
#include <string>

#include "rtype/engine/ProjectSettings.hpp"

namespace rtype_duel {

/// @brief Runs a player's window until it is closed or Escape is pressed.
/// @param projectSettings The platform, renderer and title to use.
/// @param host The server's host name or address.
/// @param port The server's UDP port.
/// @return The exit code of the App.
int runClient(const rtype::engine::ProjectSettings& projectSettings, const std::string& host, std::uint16_t port);

}  // namespace rtype_duel
