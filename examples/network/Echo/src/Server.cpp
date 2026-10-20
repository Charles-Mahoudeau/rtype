/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Server
*/

#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/server/NetServer.hpp"
#include "rtype/network/server/RoomMessages.hpp"

using namespace rtype::network;

namespace {
std::optional<std::uint16_t> parsePort(const std::string& text) {
    std::uint16_t port = 0;
    const char* const last = std::to_address(text.end());
    const auto [end, error] = std::from_chars(text.data(), last, port);
    if (error != std::errc{} || end != last) {
        return std::nullopt;
    }
    return port;
}
}  // namespace

int main(const int argc, char** argv) {
    const std::span<const char* const> argSpan{argv, static_cast<std::size_t>(argc)};
    const std::vector<std::string> args{argSpan.begin(), argSpan.end()};
    NetworkSettings settings;
    if (args.size() > 1) {
        const auto port = parsePort(args.at(1));
        if (!port) {
            std::cerr << "invalid port " << args.at(1) << std::endl;
            return 1;
        }
        settings.port = *port;
    }
    try {
        server::NetServer netServer{settings};
        std::cout << "Echo server on port " << netServer.localPort() << std::endl;
        while (true) {
            netServer.poll(Clock::now());
            for (auto& event : netServer.takeEvents()) {
                if (const auto* joined = std::get_if<server::ClientJoined>(&event)) {
                    std::cout << "client " << joined->client << " joined" << std::endl;
                } else if (auto* message = std::get_if<server::ClientMessage>(&event)) {
                    std::cout << "echo " << message->payload.size() << " bytes to client " << message->client
                              << std::endl;
                    netServer.execute(server::SendMessage{
                        .client = message->client,
                        .channel = message->channel,
                        .payload = std::move(message->payload),
                    });
                } else if (const auto* left = std::get_if<server::ClientLeft>(&event)) {
                    std::cout << "client " << left->client << " left" << std::endl;
                }
            }
            netServer.flush(Clock::now());
            std::this_thread::sleep_for(std::chrono::milliseconds{16});
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}
