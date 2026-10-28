/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <system_error>

#include "Client.hpp"
#include "Constants.hpp"
#include "Server.hpp"
#include "rtype/engine/Engine.hpp"

// A two-player duel, the most basic game the engine can run: each player moves a ship in their half of the arena and
// fires lasers; the first laser that touches the other ship wins. Obstacles in the middle stop the lasers, and the
// ships slide along them. The server simulates everything with the physics plugin and replicates Ship, Laser,
// Obstacle and Winner (the physics replicates Collider and RigidBody); the clients send their inputs and draw what
// they receive, animated by the sprite plugin, over a space background whose layers scroll at their own speed (a
// Tiled map), where a planet passes now and then; a ship that is hit explodes. Each player's name is written at the top
// of their half, and the winner in the middle at the end (MSDF text). F3 shows the colliders. One shot in ten
// misfires: the server fires nothing and replicates a Misfire instead. The clients play a looping music, and a sound
// for every laser and misfire, panned where it happened, with a random pitch and volume.
//
// The code is split by role: Game (shared rules), Server, Client (wiring) and the client systems by topic (Controls,
// Sounds, Sprites, Background, Hud).
//
//   rtype_duel server [port]           headless server, waits for two clients
//   rtype_duel client [host] [port]    a player (default 127.0.0.1:4242)

namespace {

/// @return @p text as a port, or the default one if it is empty or not a number in range.
std::uint16_t toPort(std::string_view text) {
    std::uint16_t port = 0;
    const std::string digits{text};
    const char* const last = std::to_address(digits.end());
    const auto [end, error] = std::from_chars(digits.c_str(), last, port);
    return error == std::errc{} && end == last && port != 0 ? port : rtype_duel::kDefaultPort;
}

}  // namespace

int main(const int argc, char** argv) {
    try {
        const std::span args{argv, static_cast<std::size_t>(argc)};
        const rtype::engine::ProjectSettings projectSettings =
            rtype::engine::Engine::loadProjectSettings("game/project.luau");

        const auto arg = [&args](const std::size_t index, const std::string_view fallback) {
            return index < args.size() ? std::string_view{args.subspan(index).front()} : fallback;
        };
        const std::string_view mode = arg(1, "client");
        if (mode == "server") {
            return rtype_duel::runServer(toPort(arg(2, "")));
        }
        if (mode == "client") {
            return rtype_duel::runClient(projectSettings, std::string{arg(2, "127.0.0.1")}, toPort(arg(3, "")));
        }
        std::cerr << "Usage: rtype_duel server [port] | rtype_duel client [host] [port]\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
