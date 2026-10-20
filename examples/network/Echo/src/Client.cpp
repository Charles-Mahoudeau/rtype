/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Client
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
#include <variant>
#include <vector>

#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/client/ClientEvent.hpp"
#include "rtype/network/client/NetClient.hpp"
#include "rtype/network/io/Address.hpp"

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
    const std::string host = args.size() > 1 ? args.at(1) : "127.0.0.1";
    std::uint16_t port = 4242;
    if (args.size() > 2) {
        const auto parsed = parsePort(args.at(2));
        if (!parsed) {
            std::cerr << "invalid port " << args.at(2) << std::endl;
            return 1;
        }
        port = *parsed;
    }
    const auto address = io::Address::resolve(host, port);
    if (!address) {
        std::cerr << "cannot resolve " << host << std::endl;
        return 1;
    }
    try {
        client::NetClient netClient{*address, NetworkSettings{}};
        netClient.connect(Clock::now());
        const Bytes payload{std::byte{0x20}, std::byte{'h'}, std::byte{'i'}};
        bool isSent = false;
        const auto deadline = Clock::now() + std::chrono::seconds{10};
        while (Clock::now() < deadline) {
            netClient.poll(Clock::now());
            for (const auto& event : netClient.takeEvents()) {
                if (std::holds_alternative<client::ConnectionAccepted>(event)) {
                    std::cout << "connected" << std::endl;
                    netClient.sendReady();
                    isSent = netClient.send(Channel::kReliable, payload);
                } else if (const auto* message = std::get_if<client::ServerMessage>(&event)) {
                    std::cout << "echoed back " << message->payload.size() << " bytes" << std::endl;
                    netClient.disconnect(Clock::now());
                    return 0;
                } else if (std::holds_alternative<client::ConnectFailed>(event)) {
                    std::cerr << "connection failed" << std::endl;
                    return 1;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{16});
        }
        std::cerr << (isSent ? "no echo" : "not connected") << std::endl;
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
    }
    return 1;
}
