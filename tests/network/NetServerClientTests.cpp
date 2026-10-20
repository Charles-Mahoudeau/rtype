/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NetServerClientTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <thread>
#include <variant>
#include <vector>

#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/client/ClientCore.hpp"
#include "rtype/network/client/NetClient.hpp"
#include "rtype/network/exceptions/NetworkExceptions.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/network/io/UdpSocket.hpp"
#include "rtype/network/server/NetServer.hpp"

namespace network = rtype::network;
namespace client = rtype::network::client;
namespace server = rtype::network::server;

namespace {
constexpr std::uint8_t kGameType = 0x20;

network::NetworkSettings makeSettings() {
    network::NetworkSettings settings;
    settings.port = 0;
    return settings;
}

network::io::Address loopback(const std::uint16_t port) {
    return network::io::Address::resolve("127.0.0.1", port).value();
}

/// @brief Polls both ends until `done` is true or two seconds pass.
bool pollUntil(server::NetServer& netServer, client::NetClient& netClient, const std::function<bool()>& done) {
    const auto deadline = network::Clock::now() + std::chrono::seconds{2};
    while (network::Clock::now() < deadline) {
        netServer.poll(network::Clock::now());
        netClient.poll(network::Clock::now());
        if (done()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    return false;
}

template <typename E, typename V>
const E* find(const std::vector<V>& events) {
    for (const auto& event : events) {
        if (const auto* found = std::get_if<E>(&event)) {
            return found;
        }
    }
    return nullptr;
}
}  // namespace

TEST(NetServerClient, ClientConnectsAndTheServerEchoesAReliableMessage) {
    server::NetServer netServer{makeSettings()};
    client::NetClient netClient{loopback(netServer.localPort()), makeSettings()};

    netClient.connect(network::Clock::now());
    ASSERT_TRUE(
        pollUntil(netServer, netClient, [&] { return netClient.core().state() == client::ClientState::kConnected; }));
    netClient.sendReady();

    std::vector<server::ServerEvent> serverEvents;
    ASSERT_TRUE(pollUntil(netServer, netClient, [&] {
        for (auto& event : netServer.takeEvents()) {
            serverEvents.push_back(std::move(event));
        }
        return find<server::ClientJoined>(serverEvents) != nullptr;
    }));
    const network::ClientId joinedClient = find<server::ClientJoined>(serverEvents)->client;

    const network::Bytes payload{static_cast<std::byte>(kGameType), std::byte{1}, std::byte{2}, std::byte{3}};
    ASSERT_TRUE(netClient.send(network::Channel::kReliable, payload));
    ASSERT_TRUE(pollUntil(netServer, netClient, [&] {
        for (auto& event : netServer.takeEvents()) {
            serverEvents.push_back(std::move(event));
        }
        return find<server::ClientMessage>(serverEvents) != nullptr;
    }));
    const auto* received = find<server::ClientMessage>(serverEvents);
    ASSERT_NE(received, nullptr);
    EXPECT_EQ(received->client, joinedClient);
    EXPECT_EQ(received->channel, network::Channel::kReliable);
    EXPECT_EQ(received->payload, payload);

    netServer.execute(
        server::SendMessage{.client = received->client, .channel = received->channel, .payload = received->payload});
    netServer.flush(network::Clock::now());

    std::vector<client::ClientEvent> clientEvents;
    ASSERT_TRUE(pollUntil(netServer, netClient, [&] {
        for (auto& event : netClient.takeEvents()) {
            clientEvents.push_back(std::move(event));
        }
        return find<client::ServerMessage>(clientEvents) != nullptr;
    }));
    EXPECT_EQ(find<client::ServerMessage>(clientEvents)->payload, payload);
}

TEST(NetServerClient, ClientIgnoresDatagramsFromAnotherAddress) {
    server::NetServer netServer{makeSettings()};
    client::NetClient netClient{loopback(netServer.localPort()), makeSettings()};
    network::io::UdpSocket stranger{0};

    netClient.connect(network::Clock::now());
    ASSERT_EQ(netClient.core().state(), client::ClientState::kRequesting);

    // A Challenge would move the client to kResponding if it came from the server.
    stranger.sendTo(loopback(netClient.localPort()),
                    network::protocol::PacketCodec::encode(network::protocol::Challenge{.challenge = 42}));
    for (int i = 0; i < 50; ++i) {
        netClient.poll(network::Clock::now());
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    EXPECT_EQ(netClient.core().state(), client::ClientState::kRequesting);

    // The real server still gets through.
    ASSERT_TRUE(
        pollUntil(netServer, netClient, [&] { return netClient.core().state() == client::ClientState::kConnected; }));
}

TEST(NetServerClient, BindingAnUsedPortThrows) {
    server::NetServer first{makeSettings()};
    auto settings = makeSettings();
    settings.port = first.localPort();
    EXPECT_THROW(server::NetServer second{settings}, network::exceptions::SocketException);
}
