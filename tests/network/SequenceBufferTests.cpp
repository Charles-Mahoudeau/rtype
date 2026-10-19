/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SequenceBufferTests
*/

#include <gtest/gtest.h>

#include <cstdint>

#include "rtype/network/protocol/SequenceBuffer.hpp"

namespace protocol = rtype::network::protocol;

namespace {
struct Record {
    int value{0};
    bool flag{false};
};

constexpr std::size_t kSize = 16;
using Buffer = protocol::SequenceBuffer<Record, kSize>;
}  // namespace

TEST(SequenceBuffer, NewBufferIsEmpty) {
    Buffer buffer;
    const Buffer& constBuffer = buffer;

    EXPECT_EQ(buffer.find(0), nullptr);
    EXPECT_EQ(constBuffer.find(0), nullptr);
    for (std::uint32_t s = 0; s < 65536; s += 97) {
        EXPECT_EQ(buffer.find(static_cast<std::uint16_t>(s)), nullptr);
    }
}

TEST(SequenceBuffer, InsertThenFind) {
    Buffer buffer;

    buffer.insert(5).value = 42;

    const Record* found = buffer.find(5);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->value, 42);
    EXPECT_EQ(buffer.find(6), nullptr);
    EXPECT_EQ(std::as_const(buffer).find(5), found);
}

TEST(SequenceBuffer, InsertZeroIsFound) {
    Buffer buffer;

    buffer.insert(0).value = 1;

    ASSERT_NE(buffer.find(0), nullptr);
    EXPECT_EQ(buffer.find(0)->value, 1);
}

TEST(SequenceBuffer, InsertResetsReusedSlot) {
    Buffer buffer;
    Record& first = buffer.insert(3);
    first.value = 9;
    first.flag = true;

    Record& second = buffer.insert(3);

    EXPECT_EQ(second.value, 0);
    EXPECT_FALSE(second.flag);
}

TEST(SequenceBuffer, InsertOfNextLapForgetsOlderNumber) {
    Buffer buffer;
    buffer.insert(4).value = 1;

    buffer.insert(4 + kSize).value = 2;

    EXPECT_EQ(buffer.find(4), nullptr);
    ASSERT_NE(buffer.find(4 + kSize), nullptr);
    EXPECT_EQ(buffer.find(4 + kSize)->value, 2);
}

TEST(SequenceBuffer, InsertOfNextLapResetsValue) {
    Buffer buffer;
    buffer.insert(4).value = 7;

    EXPECT_EQ(buffer.insert(4 + kSize).value, 0);
}

TEST(SequenceBuffer, RemoveEmptiesSlot) {
    Buffer buffer;
    buffer.insert(8).value = 1;

    buffer.remove(8);

    EXPECT_EQ(buffer.find(8), nullptr);
}

TEST(SequenceBuffer, RemoveOnEmptySlotDoesNothing) {
    Buffer buffer;

    buffer.remove(8);

    EXPECT_EQ(buffer.find(8), nullptr);
}

TEST(SequenceBuffer, RemoveKeepsNewerEntryInSlot) {
    Buffer buffer;
    buffer.insert(2).value = 1;
    buffer.insert(2 + kSize).value = 2;

    buffer.remove(2);

    ASSERT_NE(buffer.find(2 + kSize), nullptr);
    EXPECT_EQ(buffer.find(2 + kSize)->value, 2);
}

TEST(SequenceBuffer, ReinsertAfterRemove) {
    Buffer buffer;
    buffer.insert(2).value = 1;
    buffer.remove(2);

    EXPECT_EQ(buffer.insert(2).value, 0);
    EXPECT_NE(buffer.find(2), nullptr);
}

TEST(SequenceBuffer, WorksAcrossTheWrap) {
    Buffer buffer;
    std::uint16_t sequence = 65530;
    for (int i = 0; i < 17; ++i, ++sequence) {
        buffer.insert(sequence).value = i;
    }

    // 17 inserts (65530 .. 65535, 0 .. 10) over 16 slots: only 65530 was overwritten, by 10.
    EXPECT_EQ(buffer.find(65530), nullptr);
    for (std::uint32_t i = 1; i < 17; ++i) {
        const auto s = static_cast<std::uint16_t>(65530 + i);
        ASSERT_NE(buffer.find(s), nullptr) << "sequence " << s;
        EXPECT_EQ(buffer.find(s)->value, static_cast<int>(i));
    }

    buffer.remove(65530);
    ASSERT_NE(buffer.find(10), nullptr);

    buffer.remove(65535);
    EXPECT_EQ(buffer.find(65535), nullptr);
    EXPECT_NE(buffer.find(0), nullptr);
}

TEST(SequenceBuffer, NeighboursAcrossTheWrapKeepTheirOwnSlot) {
    Buffer buffer;
    buffer.insert(65535).value = 1;
    buffer.insert(0).value = 2;

    ASSERT_NE(buffer.find(65535), nullptr);
    ASSERT_NE(buffer.find(0), nullptr);
    EXPECT_EQ(buffer.find(65535)->value, 1);
    EXPECT_EQ(buffer.find(0)->value, 2);
}
