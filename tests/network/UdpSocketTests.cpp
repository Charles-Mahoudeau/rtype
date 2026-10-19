/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UdpSocketTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <vector>

#include "rtype/network/Types.hpp"
#include "rtype/network/exceptions/NetworkExceptions.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/network/io/UdpSocket.hpp"
#include "rtype/network/protocol/Constants.hpp"

namespace io = rtype::network::io;
using rtype::network::Bytes;

namespace {
Bytes makeBytes(const std::size_t size, const std::byte value = std::byte{0x2A}) { return Bytes(size, value); }

io::Address loopback(const io::UdpSocket& socket) {
    return io::Address::resolve("127.0.0.1", socket.localPort()).value();
}

/// @brief Polls `socket` until at least `count` datagrams arrived, or a timeout passes.
std::vector<io::Datagram> receive(io::UdpSocket& socket, const std::size_t count) {
    std::vector<io::Datagram> all;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
    while (all.size() < count && std::chrono::steady_clock::now() < deadline) {
        socket.poll();
        for (auto& datagram : socket.takeReceived()) {
            all.push_back(std::move(datagram));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    return all;
}

/// @brief Polls for a short while, for tests that expect nothing to arrive.
void settle(io::UdpSocket& socket) {
    for (int i = 0; i < 50; ++i) {
        socket.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
}
}  // namespace

TEST(UdpSocket, ZeroPortPicksAFreePort) {
    const io::UdpSocket socket{0};
    EXPECT_NE(socket.localPort(), 0);
}

TEST(UdpSocket, TwoSocketsExchangeDatagramsBothWays) {
    io::UdpSocket first{0};
    io::UdpSocket second{0};
    const auto payload = makeBytes(10);

    first.sendTo(loopback(second), payload);
    const auto atSecond = receive(second, 1);
    ASSERT_EQ(atSecond.size(), 1U);
    EXPECT_EQ(atSecond[0].bytes, payload);
    EXPECT_EQ(atSecond[0].from, loopback(first));

    second.sendTo(atSecond[0].from, makeBytes(3));
    const auto atFirst = receive(first, 1);
    ASSERT_EQ(atFirst.size(), 1U);
    EXPECT_EQ(atFirst[0].bytes.size(), 3U);
    EXPECT_EQ(first.stats().received, 1U);
}

TEST(UdpSocket, TakeReceivedEmptiesTheList) {
    io::UdpSocket first{0};
    io::UdpSocket second{0};
    first.sendTo(loopback(second), makeBytes(1));
    ASSERT_EQ(receive(second, 1).size(), 1U);
    EXPECT_TRUE(second.takeReceived().empty());
}

TEST(UdpSocket, MaxSizeDatagramIsKept) {
    io::UdpSocket first{0};
    io::UdpSocket second{0};
    first.sendTo(loopback(second), makeBytes(rtype::network::protocol::kMaxPacketSize));
    const auto received = receive(second, 1);
    ASSERT_EQ(received.size(), 1U);
    EXPECT_EQ(received[0].bytes.size(), rtype::network::protocol::kMaxPacketSize);
}

TEST(UdpSocket, OversizedDatagramIsDroppedAndCounted) {
    io::UdpSocket first{0};
    io::UdpSocket second{0};
    first.sendTo(loopback(second), makeBytes(rtype::network::protocol::kMaxPacketSize + 1));
    settle(second);
    EXPECT_TRUE(second.takeReceived().empty());
    EXPECT_EQ(second.stats().oversized, 1U);
    EXPECT_EQ(second.stats().received, 0U);
}

TEST(UdpSocket, KeepsReceivingAfterSendingToAClosedPort) {
    io::UdpSocket server{0};
    std::uint16_t closedPort = 0;
    {
        const io::UdpSocket closed{0};
        closedPort = closed.localPort();
    }
    // On Windows this makes the next receive fail with connection_reset.
    server.sendTo(io::Address::resolve("127.0.0.1", closedPort).value(), makeBytes(4));
    settle(server);

    io::UdpSocket client{0};
    client.sendTo(loopback(server), makeBytes(5));
    const auto received = receive(server, 1);
    ASSERT_EQ(received.size(), 1U);
    EXPECT_EQ(received[0].bytes.size(), 5U);
}

TEST(UdpSocket, BindingAPortInUseThrows) {
    const io::UdpSocket first{0};
    EXPECT_THROW(io::UdpSocket{first.localPort()}, rtype::network::exceptions::SocketException);
}

TEST(UdpSocket, SendingToAnIpv6AddressIsCountedNotThrown) {
    io::UdpSocket socket{0};
    socket.sendTo(io::Address::resolve("::1", 4242).value(), makeBytes(1));
    EXPECT_EQ(socket.stats().sendErrors, 1U);
}

TEST(UdpSocket, StopMakesRunReturn) {
    io::UdpSocket socket{0};
    std::thread stopper{[&socket] {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        socket.stop();
    }};
    socket.run();
    stopper.join();
    SUCCEED();
}
