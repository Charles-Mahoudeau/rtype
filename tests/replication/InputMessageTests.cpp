/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputMessageTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "rtype/network/Types.hpp"
#include "rtype/replication/MessageType.hpp"
#include "rtype/replication/input/InputKind.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/input/InputMessage.hpp"

namespace {
using rtype::replication::input::decodeInput;
using rtype::replication::input::encodeInput;
using rtype::replication::input::InputKind;
using rtype::replication::input::InputLayout;
using rtype::replication::input::InputMessage;
using rtype::replication::input::InputSample;

// Slots: move = 0, 1; throttle = 2. Buttons: boost = 0, fire = 1.
InputLayout makeLayout() {
    InputLayout layout;
    layout.add("move", InputKind::kVector2);
    layout.add("fire", InputKind::kButton);
    layout.add("throttle", InputKind::kAxis);
    layout.add("boost", InputKind::kButton);
    return layout;
}

InputSample makeSample(const std::uint32_t sequence, const std::uint32_t buttons) {
    InputSample sample;
    sample.sequence = sequence;
    sample.buttons = buttons;
    sample.axes[0] = 1.0F;
    sample.axes[1] = -1.0F;
    sample.axes[2] = 0.5F;
    return sample;
}

std::span<const std::byte> view(const rtype::network::Bytes& bytes) { return bytes; }
}  // namespace

TEST(InputMessage, RoundTripsOneInput) {
    const InputLayout layout = makeLayout();
    const InputMessage message{.lastAppliedSnapshot = 42, .inputs = {makeSample(7, 0b10)}};

    const auto bytes = encodeInput(message, layout);
    EXPECT_EQ(bytes.size(), 6U + 8U + 6U);
    EXPECT_EQ(bytes[0], static_cast<std::byte>(rtype::replication::MessageType::kInput));

    const auto decoded = decodeInput(view(bytes), layout);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->lastAppliedSnapshot, 42U);
    ASSERT_EQ(decoded->inputs.size(), 1U);
    EXPECT_EQ(decoded->inputs[0].sequence, 7U);
    EXPECT_EQ(decoded->inputs[0].buttons, 0b10U);
    EXPECT_FLOAT_EQ(decoded->inputs[0].axes[0], 1.0F);
    EXPECT_FLOAT_EQ(decoded->inputs[0].axes[1], -1.0F);
    EXPECT_NEAR(decoded->inputs[0].axes[2], 0.5F, 1.0F / 32767.0F);
}

TEST(InputMessage, RoundTripsThreeInputs) {
    const InputLayout layout = makeLayout();
    const InputMessage message{.lastAppliedSnapshot = 1,
                               .inputs = {makeSample(5, 0), makeSample(6, 1), makeSample(7, 3)}};

    const auto decoded = decodeInput(view(encodeInput(message, layout)), layout);
    ASSERT_TRUE(decoded);
    ASSERT_EQ(decoded->inputs.size(), 3U);
    EXPECT_EQ(decoded->inputs[0].sequence, 5U);
    EXPECT_EQ(decoded->inputs[2].sequence, 7U);
    EXPECT_EQ(decoded->inputs[2].buttons, 3U);
}

TEST(InputMessage, RefusesCountsZeroAndFour) {
    const InputLayout layout = makeLayout();
    const InputMessage none{.lastAppliedSnapshot = 0, .inputs = {}};
    EXPECT_FALSE(decodeInput(view(encodeInput(none, layout)), layout));

    const InputMessage four{.lastAppliedSnapshot = 0,
                            .inputs = {makeSample(1, 0), makeSample(2, 0), makeSample(3, 0), makeSample(4, 0)}};
    EXPECT_FALSE(decodeInput(view(encodeInput(four, layout)), layout));
}

TEST(InputMessage, RefusesSequencesThatDoNotStrictlyIncrease) {
    const InputLayout layout = makeLayout();
    const InputMessage same{.lastAppliedSnapshot = 0, .inputs = {makeSample(4, 0), makeSample(4, 0)}};
    EXPECT_FALSE(decodeInput(view(encodeInput(same, layout)), layout));
    const InputMessage back{.lastAppliedSnapshot = 0, .inputs = {makeSample(5, 0), makeSample(4, 0)}};
    EXPECT_FALSE(decodeInput(view(encodeInput(back, layout)), layout));
}

TEST(InputMessage, RefusesAButtonBitBeyondTheLayout) {
    const InputLayout layout = makeLayout();
    const InputMessage message{.lastAppliedSnapshot = 0, .inputs = {makeSample(1, 0b100)}};
    EXPECT_FALSE(decodeInput(view(encodeInput(message, layout)), layout));
}

TEST(InputMessage, RefusesMissingAndExtraBytes) {
    const InputLayout layout = makeLayout();
    const InputMessage message{.lastAppliedSnapshot = 0, .inputs = {makeSample(1, 0)}};
    auto bytes = encodeInput(message, layout);

    for (std::size_t size = 0; size < bytes.size(); ++size) {
        EXPECT_FALSE(decodeInput(std::span{bytes}.first(size), layout)) << size;
    }
    bytes.push_back(std::byte{0});
    EXPECT_FALSE(decodeInput(view(bytes), layout));
}

TEST(InputMessage, RefusesAnotherMessageType) {
    const InputLayout layout = makeLayout();
    const InputMessage message{.lastAppliedSnapshot = 0, .inputs = {makeSample(1, 0)}};
    auto bytes = encodeInput(message, layout);
    bytes[0] = std::byte{0x30};
    EXPECT_FALSE(decodeInput(view(bytes), layout));
}

TEST(InputMessage, ReadsMinus32768AsMinusOne) {
    const InputLayout layout = makeLayout();
    const InputMessage message{.lastAppliedSnapshot = 0, .inputs = {makeSample(1, 0)}};
    auto bytes = encodeInput(message, layout);
    // The first axis slot starts after the type, the tick, the count, the sequence and the buttons.
    bytes[14] = std::byte{0x00};
    bytes[15] = std::byte{0x80};

    const auto decoded = decodeInput(view(bytes), layout);
    ASSERT_TRUE(decoded);
    EXPECT_FLOAT_EQ(decoded->inputs[0].axes[0], -1.0F);
}

TEST(InputMessage, QuantizesToPlusMinus32767) {
    const InputLayout layout = makeLayout();
    const InputMessage message{.lastAppliedSnapshot = 0, .inputs = {makeSample(1, 0)}};
    const auto bytes = encodeInput(message, layout);

    EXPECT_EQ(bytes[14], std::byte{0xFF});
    EXPECT_EQ(bytes[15], std::byte{0x7F});
    EXPECT_EQ(bytes[16], std::byte{0x01});
    EXPECT_EQ(bytes[17], std::byte{0x80});
}
