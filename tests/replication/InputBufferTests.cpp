/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputBufferTests
*/

#include <gtest/gtest.h>

#include <cstdint>

#include "rtype/replication/input/InputBuffer.hpp"
#include "rtype/replication/input/InputMessage.hpp"

namespace {
using rtype::replication::input::InputBuffer;
using rtype::replication::input::InputSample;

InputSample makeSample(const std::uint32_t sequence) {
    InputSample sample;
    sample.sequence = sequence;
    sample.buttons = sequence;
    return sample;
}
}  // namespace

TEST(InputBuffer, IsNeutralBeforeAnyInput) {
    InputBuffer buffer;
    EXPECT_EQ(buffer.next().sequence, 0U);
    EXPECT_EQ(buffer.next().buttons, 0U);
    EXPECT_EQ(buffer.getLastProcessed(), 0U);
}

TEST(InputBuffer, HandsOutInputsInSequenceOrder) {
    InputBuffer buffer;
    buffer.push(makeSample(3));
    buffer.push(makeSample(1));
    buffer.push(makeSample(2));

    EXPECT_EQ(buffer.next().sequence, 1U);
    EXPECT_EQ(buffer.next().sequence, 2U);
    EXPECT_EQ(buffer.next().sequence, 3U);
    EXPECT_EQ(buffer.getLastProcessed(), 3U);
}

TEST(InputBuffer, IgnoresDuplicatesAndAppliedSequences) {
    InputBuffer buffer;
    buffer.push(makeSample(2));
    buffer.push(makeSample(2));
    EXPECT_EQ(buffer.next().sequence, 2U);

    buffer.push(makeSample(2));
    buffer.push(makeSample(1));
    EXPECT_EQ(buffer.next().sequence, 2U);  // nothing new: repeated
    EXPECT_EQ(buffer.getLastProcessed(), 2U);
}

TEST(InputBuffer, AcceptsSequenceZeroAsTheFirstInput) {
    InputBuffer buffer;
    buffer.push(makeSample(0));
    buffer.push(makeSample(1));
    EXPECT_EQ(buffer.next().sequence, 0U);
    EXPECT_EQ(buffer.next().sequence, 1U);
}

TEST(InputBuffer, RepeatsTheLastInputWithoutAdvancing) {
    InputBuffer buffer;
    buffer.push(makeSample(5));
    EXPECT_EQ(buffer.next().buttons, 5U);
    EXPECT_EQ(buffer.next().buttons, 5U);
    EXPECT_EQ(buffer.next().buttons, 5U);
    EXPECT_EQ(buffer.getLastProcessed(), 5U);
}

TEST(InputBuffer, DropsTheOldestDownToTwoOnOverflow) {
    InputBuffer buffer;
    for (std::uint32_t sequence = 1; sequence <= 4; ++sequence) {
        buffer.push(makeSample(sequence));
    }
    buffer.push(makeSample(5));

    EXPECT_EQ(buffer.next().sequence, 4U);
    EXPECT_EQ(buffer.next().sequence, 5U);
    EXPECT_EQ(buffer.getLastProcessed(), 5U);
}

TEST(InputBuffer, ResetEmptiesAndMakesTheInputNeutral) {
    InputBuffer buffer;
    buffer.push(makeSample(1));
    buffer.push(makeSample(2));
    EXPECT_EQ(buffer.next().sequence, 1U);

    buffer.reset();
    EXPECT_EQ(buffer.next().sequence, 0U);
    EXPECT_EQ(buffer.next().buttons, 0U);
    EXPECT_EQ(buffer.getLastProcessed(), 0U);

    buffer.push(makeSample(1));
    EXPECT_EQ(buffer.next().sequence, 1U);
}
