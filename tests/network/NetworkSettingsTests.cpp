/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NetworkSettingsTests
*/

#include <gtest/gtest.h>

#include <chrono>

#include "rtype/network/NetworkSettings.hpp"

TEST(NetworkSettings, DefaultsMatchTheDesign) {
    const rtype::network::NetworkSettings settings;

    EXPECT_EQ(settings.port, 4242);
    EXPECT_EQ(settings.maxClients, 64U);
    EXPECT_EQ(settings.timeout, std::chrono::seconds{2});
    EXPECT_EQ(settings.keepalive, std::chrono::milliseconds{100});
    EXPECT_EQ(settings.minResendInterval, std::chrono::milliseconds{50});
    EXPECT_EQ(settings.initialRtt, std::chrono::milliseconds{100});
    EXPECT_EQ(settings.handshakeRetry, std::chrono::milliseconds{100});
    EXPECT_EQ(settings.handshakeTimeout, std::chrono::seconds{5});
    EXPECT_EQ(settings.challengeExpiry, std::chrono::seconds{5});
    EXPECT_EQ(settings.maxPendingChallenges, 256U);
}

TEST(NetworkSettings, FieldsCanBeOverridden) {
    rtype::network::NetworkSettings settings;
    settings.port = 5000;
    settings.timeout = std::chrono::milliseconds{500};

    EXPECT_EQ(settings.port, 5000);
    EXPECT_EQ(settings.timeout, std::chrono::milliseconds{500});
    EXPECT_EQ(settings.maxClients, 64U);
}
