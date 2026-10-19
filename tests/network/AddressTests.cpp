/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AddressTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_set>

#include "rtype/network/io/Address.hpp"

using rtype::network::io::Address;

namespace {
constexpr std::uint16_t kPort = 4242;

Address ipv4(const std::uint16_t port) { return Address::resolve("127.0.0.1", port).value(); }
}  // namespace

TEST(Address, DefaultIsUnspecifiedIpv4WithPortZero) {
    const Address address;
    EXPECT_EQ(address.port(), 0);
    EXPECT_EQ(address.string(), "[::]:0");
}

TEST(Address, ResolvesIpv4LiteralAndFormatsIt) {
    const auto address = Address::resolve("127.0.0.1", kPort);
    ASSERT_TRUE(address);
    EXPECT_EQ(address->port(), kPort);
    EXPECT_EQ(address->string(), "127.0.0.1:4242");
}

TEST(Address, ResolvesIpv6LiteralAndFormatsIt) {
    const auto address = Address::resolve("::1", kPort);
    ASSERT_TRUE(address);
    EXPECT_EQ(address->string(), "[::1]:4242");
}

TEST(Address, ResolvesLocalhost) {
    const auto address = Address::resolve("localhost", kPort);
    ASSERT_TRUE(address);
    EXPECT_EQ(address->port(), kPort);
}

TEST(Address, ResolveReturnsNulloptWhenNothingResolves) { EXPECT_FALSE(Address::resolve("host.invalid", kPort)); }

TEST(Address, EqualWhenIpAndPortAreEqual) {
    EXPECT_EQ(ipv4(kPort), ipv4(kPort));
    EXPECT_NE(ipv4(kPort), ipv4(kPort + 1));
    EXPECT_NE(ipv4(kPort), Address::resolve("127.0.0.2", kPort).value());
}

TEST(Address, HashIsConsistentWithEquality) {
    const std::hash<Address> hash;
    EXPECT_EQ(hash(ipv4(kPort)), hash(ipv4(kPort)));
    std::unordered_set<Address> set{ipv4(kPort), ipv4(kPort), ipv4(kPort + 1)};
    EXPECT_EQ(set.size(), 2U);
}
