/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AdminTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/admin/AdminClient.hpp"
#include "rtype/network/admin/AdminProtocol.hpp"
#include "rtype/network/admin/AdminServer.hpp"
#include "rtype/network/client/NetClient.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/network/io/UdpSocket.hpp"
#include "rtype/network/server/NetServer.hpp"
#include "rtype/network/simulation/LinkSimulator.hpp"

namespace network = rtype::network;
namespace admin = rtype::network::admin;
namespace client = rtype::network::client;
namespace server = rtype::network::server;
namespace simulation = rtype::network::simulation;

namespace {
constexpr const char* kToken = "s3cret";

network::NetworkSettings makeSettings() {
    network::NetworkSettings settings;
    settings.port = 0;
    return settings;
}

/// A server, its admin channel, and a tool talking to it.
struct Fixture {
    server::NetServer netServer{makeSettings()};
    admin::AdminServer adminServer{0, kToken, network::Clock::now()};
    admin::AdminClient tool{adminServer.localPort(), kToken};
    std::vector<admin::DecodedReply> replies;

    void step() {
        netServer.poll(network::Clock::now());
        adminServer.poll(network::Clock::now(), netServer, admin::GameStatus{.tick = 12, .entityCount = 3});
        for (auto& reply : tool.poll()) {
            replies.push_back(std::move(reply));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }

    /// @return The reply to the request @p id, or nothing after two seconds.
    std::optional<admin::Reply> wait(const std::uint32_t id) {
        const auto deadline = network::Clock::now() + std::chrono::seconds{2};
        while (network::Clock::now() < deadline) {
            step();
            for (const auto& reply : replies) {
                if (reply.id == id) {
                    return reply.reply;
                }
            }
        }
        return std::nullopt;
    }

    std::optional<admin::Reply> ask(const admin::Request& request) { return wait(tool.send(request)); }
};

/// Connects a game client to @p fixture and waits until the server sees it ready.
void connect(Fixture& fixture, client::NetClient& netClient) {
    netClient.connect(network::Clock::now());
    const auto deadline = network::Clock::now() + std::chrono::seconds{2};
    bool ready = false;
    while (network::Clock::now() < deadline && !ready) {
        fixture.step();
        netClient.poll(network::Clock::now());
        if (netClient.core().state() == client::ClientState::kConnected) {
            netClient.sendReady();
            ready = true;
        }
    }
    ASSERT_TRUE(ready);
    while (network::Clock::now() < deadline && fixture.netServer.clients().empty()) {
        fixture.step();
        netClient.poll(network::Clock::now());
    }
}
}  // namespace

TEST(AdminProtocol, RequestsRoundTrip) {
    const network::Bytes bytes = admin::encodeRequest(9, "tok", admin::KickRequest{.client = 4});
    const auto frame = admin::decodeRequest(bytes);
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->id, 9U);
    EXPECT_EQ(frame->token, "tok");
    ASSERT_TRUE(frame->request.has_value());
    EXPECT_EQ(std::get<admin::KickRequest>(*frame->request).client, 4U);
}

TEST(AdminProtocol, RepliesRoundTrip) {
    admin::ClientsReply list;
    list.total = 1;
    list.clients.push_back(admin::ClientEntry{.client = 2,
                                              .state = server::SessionState::kReady,
                                              .address = network::io::Address::resolve("127.0.0.1", 5000).value(),
                                              .rttMs = 12.5F,
                                              .droppedMessages = 3,
                                              .protocolErrors = 1});
    const auto decoded = admin::decodeReply(admin::encodeReply(5, list));
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->id, 5U);
    const auto& back = std::get<admin::ClientsReply>(decoded->reply);
    ASSERT_EQ(back.clients.size(), 1U);
    EXPECT_EQ(back.clients[0].client, 2U);
    EXPECT_EQ(back.clients[0].address.port(), 5000);
    EXPECT_FLOAT_EQ(back.clients[0].rttMs, 12.5F);
    EXPECT_EQ(back.clients[0].droppedMessages, 3U);
}

TEST(AdminProtocol, ExtensionRangesAreCarriedUndecoded) {
    const network::Bytes bytes =
        admin::encodeRequest(1, "t", admin::ExtensionRequest{.type = admin::kWorldFirst, .body = {std::byte{7}}});
    const auto frame = admin::decodeRequest(bytes);
    ASSERT_TRUE(frame.has_value() && frame->request.has_value());
    const auto& extension = std::get<admin::ExtensionRequest>(*frame->request);
    EXPECT_EQ(extension.type, admin::kWorldFirst);
    ASSERT_EQ(extension.body.size(), 1U);
}

TEST(AdminProtocol, MalformedBytesAreRejectedNotCrashing) {
    // Every prefix of valid datagrams, and every single-byte flip of them, decodes or is refused; none crashes.
    const std::vector<network::Bytes> samples{admin::encodeRequest(1, "token", admin::KickRequest{.client = 1}),
                                              admin::encodeReply(1, admin::StatusReply{}),
                                              admin::encodeReply(1, admin::ClientsReply{})};
    for (const network::Bytes& sample : samples) {
        for (std::size_t size = 0; size <= sample.size(); ++size) {
            const std::span<const std::byte> prefix{sample.data(), size};
            static_cast<void>(admin::decodeRequest(prefix));
            static_cast<void>(admin::decodeReply(prefix));
        }
        for (std::size_t i = 0; i < sample.size(); ++i) {
            network::Bytes copy = sample;
            copy[i] = static_cast<std::byte>(~static_cast<std::uint8_t>(copy[i]));
            static_cast<void>(admin::decodeRequest(copy));
            static_cast<void>(admin::decodeReply(copy));
        }
    }
    EXPECT_FALSE(admin::decodeRequest({}).has_value());
    EXPECT_FALSE(admin::decodeReply({}).has_value());
    // A kick with a short body is malformed, with its id kept so it can be answered.
    network::Bytes shortKick = admin::encodeRequest(3, "t", admin::KickRequest{.client = 1});
    shortKick.pop_back();
    const auto frame = admin::decodeRequest(shortKick);
    ASSERT_TRUE(frame.has_value());
    EXPECT_FALSE(frame->request.has_value());
    EXPECT_EQ(frame->problem, admin::ErrorCode::kMalformed);
}

TEST(AdminServer, HelloChecksTheToken) {
    Fixture fixture;
    const auto reply = fixture.ask(admin::HelloRequest{});
    ASSERT_TRUE(reply.has_value());
    EXPECT_EQ(std::get<admin::HelloReply>(*reply).version, admin::kProtocolVersion);
}

TEST(AdminServer, ABadTokenGetsOnlyAnError) {
    Fixture fixture;
    admin::AdminClient intruder{fixture.adminServer.localPort(), "wrong"};
    const std::uint32_t id = intruder.send(admin::KickRequest{.client = 1});
    std::optional<admin::Reply> reply;
    const auto deadline = network::Clock::now() + std::chrono::seconds{2};
    while (!reply && network::Clock::now() < deadline) {
        fixture.step();
        for (auto& answer : intruder.poll()) {
            if (answer.id == id) {
                reply = answer.reply;
            }
        }
    }
    ASSERT_TRUE(reply.has_value());
    EXPECT_EQ(std::get<admin::ErrorReply>(*reply).code, admin::ErrorCode::kBadToken);
}

TEST(AdminServer, GarbageDatagramsAreDroppedAndTheServerKeepsAnswering) {
    Fixture fixture;
    network::io::UdpSocket raw{0, true};
    const auto target = network::io::Address::resolve("127.0.0.1", fixture.adminServer.localPort()).value();
    const network::Bytes garbage{std::byte{0xFF}, std::byte{0x01}};
    raw.sendTo(target, garbage);
    raw.sendTo(target, network::Bytes{});
    for (int i = 0; i < 20; ++i) {
        fixture.step();
    }
    EXPECT_TRUE(fixture.ask(admin::HelloRequest{}).has_value());
}

TEST(AdminServer, AnUnknownTypeIsUnsupported) {
    Fixture fixture;
    const auto reply = fixture.ask(admin::ExtensionRequest{.type = admin::kWorldFirst, .body = {}});
    ASSERT_TRUE(reply.has_value());
    EXPECT_EQ(std::get<admin::ErrorReply>(*reply).code, admin::ErrorCode::kUnsupported);
}

TEST(AdminServer, AnExtensionHandlerAnswersItsRange) {
    Fixture fixture;
    fixture.adminServer.setExtensionHandler([](const admin::ExtensionRequest& request) -> std::optional<admin::Reply> {
        return admin::ExtensionReply{.type = static_cast<std::uint8_t>(request.type | admin::kReplyFlag),
                                     .body = {std::byte{42}}};
    });
    const auto reply = fixture.ask(admin::ExtensionRequest{.type = admin::kWorldFirst, .body = {}});
    ASSERT_TRUE(reply.has_value());
    const auto& extension = std::get<admin::ExtensionReply>(*reply);
    EXPECT_EQ(extension.type, admin::kWorldFirst | admin::kReplyFlag);
    EXPECT_EQ(extension.body.front(), std::byte{42});
}

TEST(AdminServer, StatusReportsTheGameAndTheSocket) {
    Fixture fixture;
    const auto reply = fixture.ask(admin::StatusRequest{});
    ASSERT_TRUE(reply.has_value());
    const auto& status = std::get<admin::StatusReply>(*reply);
    EXPECT_EQ(status.tick, 12U);
    EXPECT_EQ(status.entityCount, 3U);
    EXPECT_EQ(status.clientCount, 0U);
}

TEST(AdminServer, ListsConnectedClientsAndKicksOne) {
    Fixture fixture;
    client::NetClient netClient{network::io::Address::resolve("127.0.0.1", fixture.netServer.localPort()).value(),
                                makeSettings()};
    connect(fixture, netClient);

    const auto listed = fixture.ask(admin::ClientsRequest{});
    ASSERT_TRUE(listed.has_value());
    const auto& list = std::get<admin::ClientsReply>(*listed);
    ASSERT_EQ(list.clients.size(), 1U);
    EXPECT_EQ(list.total, 1U);
    const network::ClientId id = list.clients.front().client;
    EXPECT_NE(id, network::kNoClient);

    const auto missing = fixture.ask(admin::KickRequest{.client = id + 100});
    ASSERT_TRUE(missing.has_value());
    EXPECT_EQ(std::get<admin::ErrorReply>(*missing).code, admin::ErrorCode::kUnknownClient);

    const auto kicked = fixture.ask(admin::KickRequest{.client = id});
    ASSERT_TRUE(kicked.has_value());
    EXPECT_EQ(std::get<admin::KickReply>(*kicked).client, id);
    const auto deadline = network::Clock::now() + std::chrono::seconds{2};
    while (!fixture.netServer.clients().empty() && network::Clock::now() < deadline) {
        fixture.step();
    }
    EXPECT_TRUE(fixture.netServer.clients().empty());
}

TEST(UdpSocketStats, CountsDatagramsAndBytes) {
    Fixture fixture;
    const auto reply = fixture.ask(admin::StatusRequest{});
    ASSERT_TRUE(reply.has_value());
    EXPECT_EQ(fixture.netServer.socketStats().bytesReceived, 0U);  // the game socket saw nothing
    network::io::UdpSocket receiver{0, true};
    network::io::UdpSocket sender{0, true};
    sender.sendTo(network::io::Address::resolve("127.0.0.1", receiver.localPort()).value(),
                  network::Bytes{std::byte{1}, std::byte{2}, std::byte{3}});
    const auto deadline = network::Clock::now() + std::chrono::seconds{2};
    while (receiver.stats().received == 0 && network::Clock::now() < deadline) {
        receiver.poll();
    }
    EXPECT_EQ(sender.stats().sent, 1U);
    EXPECT_EQ(sender.stats().bytesSent, 3U);
    EXPECT_EQ(receiver.stats().bytesReceived, 3U);
}

TEST(AdminProtocol, SimulationMessagesRoundTrip) {
    const simulation::SimulationSettings wanted{.latencyMs = 120, .jitterMs = 30, .lossPercent = 2.5F};
    const auto frame =
        admin::decodeRequest(admin::encodeRequest(5, "t", admin::SimulationSetRequest{.settings = wanted}));
    ASSERT_TRUE(frame.has_value() && frame->request.has_value());
    EXPECT_EQ(std::get<admin::SimulationSetRequest>(*frame->request).settings, wanted);
    const auto get = admin::decodeRequest(admin::encodeRequest(6, "t", admin::SimulationGetRequest{}));
    ASSERT_TRUE(get.has_value() && get->request.has_value());
    EXPECT_TRUE(std::holds_alternative<admin::SimulationGetRequest>(*get->request));
    const auto reply = admin::decodeReply(admin::encodeReply(5, admin::SimulationReply{.settings = wanted}));
    ASSERT_TRUE(reply.has_value());
    EXPECT_EQ(std::get<admin::SimulationReply>(reply->reply).settings, wanted);
}

TEST(AdminProtocol, ASimulationSetWithAShortOrLongBodyIsMalformed) {
    network::Bytes bytes = admin::encodeRequest(1, "t", admin::SimulationSetRequest{});
    bytes.pop_back();
    auto frame = admin::decodeRequest(bytes);
    ASSERT_TRUE(frame.has_value());
    EXPECT_FALSE(frame->request.has_value());
    EXPECT_EQ(frame->problem, admin::ErrorCode::kMalformed);
    bytes = admin::encodeRequest(1, "t", admin::SimulationSetRequest{});
    bytes.push_back(std::byte{0});
    frame = admin::decodeRequest(bytes);
    ASSERT_TRUE(frame.has_value());
    EXPECT_FALSE(frame->request.has_value());
    // An unknown type of the simulation range is unsupported, not an extension.
    network::Bytes unknown = admin::encodeRequest(1, "t", admin::SimulationGetRequest{});
    unknown[0] = std::byte{0x1F};
    frame = admin::decodeRequest(unknown);
    ASSERT_TRUE(frame.has_value());
    EXPECT_FALSE(frame->request.has_value());
    EXPECT_EQ(frame->problem, admin::ErrorCode::kUnsupported);
}

TEST(AdminServer, SimulationIsOffUntilSetAndTheReplyShowsWhatIsInForce) {
    Fixture fixture;
    auto reply = fixture.ask(admin::SimulationGetRequest{});
    ASSERT_TRUE(reply.has_value());
    EXPECT_FALSE(std::get<admin::SimulationReply>(*reply).settings.isActive());

    reply = fixture.ask(
        admin::SimulationSetRequest{.settings = {.latencyMs = 200, .jitterMs = 60000, .lossPercent = 250.0F}});
    ASSERT_TRUE(reply.has_value());
    const auto& applied = std::get<admin::SimulationReply>(*reply).settings;
    EXPECT_EQ(applied.latencyMs, 200);
    EXPECT_EQ(applied.jitterMs, simulation::kMaxDelayMs);
    EXPECT_FLOAT_EQ(applied.lossPercent, 100.0F);
    EXPECT_EQ(fixture.netServer.simulation(), applied);

    reply = fixture.ask(admin::SimulationSetRequest{});
    ASSERT_TRUE(reply.has_value());
    EXPECT_FALSE(fixture.netServer.simulation().isActive());
}

TEST(AdminServer, ASimulationRequestNeedsTheToken) {
    Fixture fixture;
    admin::AdminClient intruder{fixture.adminServer.localPort(), "wrong"};
    const std::uint32_t id = intruder.send(admin::SimulationSetRequest{.settings = {.lossPercent = 100.0F}});
    std::optional<admin::Reply> reply;
    const auto deadline = network::Clock::now() + std::chrono::seconds{2};
    while (!reply && network::Clock::now() < deadline) {
        fixture.step();
        for (auto& answer : intruder.poll()) {
            if (answer.id == id) {
                reply = answer.reply;
            }
        }
    }
    ASSERT_TRUE(reply.has_value());
    EXPECT_EQ(std::get<admin::ErrorReply>(*reply).code, admin::ErrorCode::kBadToken);
    EXPECT_FALSE(fixture.netServer.simulation().isActive());
}

TEST(NetServerSimulation, TotalLossKeepsClientsOutUntilItIsTurnedOff) {
    Fixture fixture;
    fixture.netServer.setSimulation({.lossPercent = 100.0F});
    client::NetClient netClient{network::io::Address::resolve("127.0.0.1", fixture.netServer.localPort()).value(),
                                makeSettings()};
    netClient.connect(network::Clock::now());
    const auto blocked = network::Clock::now() + std::chrono::milliseconds{300};
    while (network::Clock::now() < blocked) {
        fixture.step();
        netClient.poll(network::Clock::now());
    }
    EXPECT_NE(netClient.core().state(), client::ClientState::kConnected);
    EXPECT_EQ(fixture.netServer.core().sessionCount(), 0U);

    fixture.netServer.setSimulation({});
    netClient.connect(network::Clock::now());
    const auto deadline = network::Clock::now() + std::chrono::seconds{3};
    while (network::Clock::now() < deadline && netClient.core().state() != client::ClientState::kConnected) {
        fixture.step();
        netClient.poll(network::Clock::now());
    }
    EXPECT_EQ(netClient.core().state(), client::ClientState::kConnected);
}
