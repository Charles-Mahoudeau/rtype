/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputLayoutTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>

#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/exceptions/ReplicationExceptions.hpp"
#include "rtype/replication/input/InputKind.hpp"
#include "rtype/replication/input/InputLayout.hpp"

namespace {
using rtype::replication::component::PlayerInput;
using rtype::replication::exceptions::InputLayoutException;
using rtype::replication::input::InputKind;
using rtype::replication::input::InputLayout;

InputLayout makeLayout() {
    InputLayout layout;
    layout.add("move", InputKind::kVector2);
    layout.add("fire", InputKind::kButton);
    layout.add("throttle", InputKind::kAxis);
    layout.add("boost", InputKind::kButton);
    return layout;
}
}  // namespace

TEST(InputLayout, GivesTheSameBitsAndSlotsWhateverTheOrderOfAdd) {
    InputLayout first = makeLayout();
    InputLayout second;
    second.add("throttle", InputKind::kAxis);
    second.add("boost", InputKind::kButton);
    second.add("move", InputKind::kVector2);
    second.add("fire", InputKind::kButton);

    for (const char* name : {"move", "fire", "throttle", "boost"}) {
        EXPECT_EQ(first.find(name), second.find(name)) << name;
    }
    // Name order: boost, fire, move, throttle.
    EXPECT_EQ(first.find("boost").index, 0U);
    EXPECT_EQ(first.find("fire").index, 1U);
    EXPECT_EQ(first.find("move").index, 0U);
    EXPECT_EQ(first.find("throttle").index, 2U);
    EXPECT_EQ(first.buttonCount(), 2U);
    EXPECT_EQ(first.axisSlotCount(), 3U);
}

TEST(InputLayout, RefusesMoreThan32Buttons) {
    InputLayout layout;
    for (int i = 0; i < 32; ++i) {
        layout.add("b" + std::to_string(i), InputKind::kButton);
    }
    EXPECT_THROW(layout.add("extra", InputKind::kButton), InputLayoutException);
    EXPECT_EQ(layout.buttonCount(), 32U);
    EXPECT_EQ(layout.actions().size(), 32U);
}

TEST(InputLayout, RefusesMoreThanMaxAxisSlots) {
    InputLayout layout;
    for (int i = 0; i < 8; ++i) {
        layout.add("v" + std::to_string(i), InputKind::kVector2);
    }
    EXPECT_EQ(layout.axisSlotCount(), 16U);
    EXPECT_THROW(layout.add("extra", InputKind::kAxis), InputLayoutException);
    EXPECT_EQ(layout.axisSlotCount(), 16U);
}

TEST(InputLayout, RefusesDeclarationsAfterFreeze) {
    InputLayout layout = makeLayout();
    layout.freeze();
    EXPECT_THROW(layout.add("extra", InputKind::kButton), InputLayoutException);
}

TEST(InputLayout, ReadsButtonsThroughPreviousButtons) {
    const InputLayout layout = makeLayout();
    PlayerInput input;
    layout.setButton(input, "fire", true);

    EXPECT_TRUE(layout.isHeld(input, "fire"));
    EXPECT_FALSE(layout.isHeld(input, "boost"));
    EXPECT_TRUE(layout.isPressed(input, "fire"));
    EXPECT_FALSE(layout.isReleased(input, "fire"));

    input.previousButtons = input.buttons;
    EXPECT_TRUE(layout.isHeld(input, "fire"));
    EXPECT_FALSE(layout.isPressed(input, "fire"));

    layout.setButton(input, "fire", false);
    EXPECT_FALSE(layout.isHeld(input, "fire"));
    EXPECT_TRUE(layout.isReleased(input, "fire"));
}

TEST(InputLayout, WritesAndReadsAxesAndVectors) {
    const InputLayout layout = makeLayout();
    PlayerInput input;
    layout.setAxis(input, "throttle", 0.5F);
    layout.setVector(input, "move", {-0.25F, 1.0F});

    EXPECT_FLOAT_EQ(layout.readAxis(input, "throttle"), 0.5F);
    const std::array<float, 2> move = layout.readVector(input, "move");
    EXPECT_FLOAT_EQ(move[0], -0.25F);
    EXPECT_FLOAT_EQ(move[1], 1.0F);
}

TEST(InputLayout, ClampsAxesAndTurnsNaNIntoZero) {
    const InputLayout layout = makeLayout();
    PlayerInput input;
    layout.setAxis(input, "throttle", 7.0F);
    EXPECT_FLOAT_EQ(layout.readAxis(input, "throttle"), 1.0F);
    layout.setAxis(input, "throttle", -7.0F);
    EXPECT_FLOAT_EQ(layout.readAxis(input, "throttle"), -1.0F);
    layout.setAxis(input, "throttle", std::numeric_limits<float>::quiet_NaN());
    EXPECT_FLOAT_EQ(layout.readAxis(input, "throttle"), 0.0F);
    layout.setVector(input, "move", {std::numeric_limits<float>::quiet_NaN(), -3.0F});
    EXPECT_EQ(layout.readVector(input, "move"), (std::array<float, 2>{0.0F, -1.0F}));
}

TEST(InputLayout, ThrowsOnAnUnknownNameOrTheWrongKind) {
    const InputLayout layout = makeLayout();
    PlayerInput input;
    EXPECT_THROW(static_cast<void>(layout.find("nope")), InputLayoutException);
    EXPECT_THROW(static_cast<void>(layout.isHeld(input, "nope")), InputLayoutException);
    EXPECT_THROW(static_cast<void>(layout.isHeld(input, "move")), InputLayoutException);
    EXPECT_THROW(static_cast<void>(layout.readAxis(input, "fire")), InputLayoutException);
    EXPECT_THROW(static_cast<void>(layout.readVector(input, "throttle")), InputLayoutException);
    EXPECT_THROW(layout.setButton(input, "move", true), InputLayoutException);
}

TEST(InputLayout, ReadsThroughAHandle) {
    const InputLayout layout = makeLayout();
    PlayerInput input;
    const auto fire = layout.find("fire");
    layout.setButton(input, fire, true);
    EXPECT_TRUE(layout.isHeld(input, fire));
    EXPECT_TRUE(layout.isPressed(input, fire));
}
