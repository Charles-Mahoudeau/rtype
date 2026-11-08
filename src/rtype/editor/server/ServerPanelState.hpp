/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ServerPanelState
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace rtype::editor::server {

/// @brief What the Server panel keeps between two frames: the fields the user types in and what it selected. Plain
/// data, so the panel's logic needs no ImGui to be tested.
struct ServerPanelState {
    std::array<char, 512> gameDirectory{};  ///< The game folder to run (default: the open game's).
    int port{4242};                         ///< The port of the server and the one clients connect to.
    std::array<char, 256> serverArgs{};     ///< More flags for the server, as typed.
    std::array<char, 64> host{'1', '2', '7', '.', '0', '.', '0', '.', '1', '\0'};  ///< The host clients connect to.
    int clientCount{2};                  ///< How many clients "Launch" starts.
    std::array<char, 256> clientArgs{};  ///< More flags for the clients, as typed.
    std::string message;                 ///< The last thing that went wrong when starting, shown in red.

    int latencyMs{0};  ///< The simulation sliders: edited here, sent on release.
    int jitterMs{0};
    float lossPercent{0.0F};

    std::uint32_t entitiesFirst{0};       ///< The first entity of the page the World tab shows.
    std::optional<std::uint64_t> entity;  ///< The server entity the World tab inspects.
    std::uint16_t firstComponent{0};      ///< The first component of the page shown.
    std::array<char, 64> spawnName{};     ///< The name of the next spawned entity.
    bool restartClients{true};            ///< Whether restarting the server starts the clients again.
    std::size_t logSeen{0};               ///< Lines of the log already shown, to scroll to new ones.
};

}  // namespace rtype::editor::server
