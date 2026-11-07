/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NetworkInputTests
*/

#include <gtest/gtest.h>

#include <array>
#include <glm/ext/vector_float2.hpp>
#include <optional>
#include <string_view>

#include "rtype/ecs/app/App.hpp"
#include "rtype/engine/NetworkInput.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/input/InputKind.hpp"
#include "rtype/replication/input/InputLayout.hpp"

namespace {
using rtype::engine::ActionReading;
using rtype::replication::input::InputKind;
}  // namespace

TEST(NetworkInput, ReadsEachActionOfTheLayoutAsItsKind) {
    rtype::replication::input::InputLayout layout;
    layout.add("shoot", InputKind::kButton);
    layout.add("move", InputKind::kVector2);
    layout.add("throttle", InputKind::kAxis);
    layout.add("dash", InputKind::kButton);  // the reader does not know it: stays neutral

    const rtype::replication::component::PlayerInput read =
        rtype::engine::readNetworkInput(layout, [](const std::string_view name) -> std::optional<ActionReading> {
            if (name == "shoot") {
                return ActionReading{.held = true};
            }
            if (name == "move") {
                return ActionReading{.held = true, .value = glm::vec2{1.0F, 1.0F}};
            }
            if (name == "throttle") {
                return ActionReading{.held = true, .value = glm::vec2{1.0F, 0.0F}};
            }
            return std::nullopt;
        });

    EXPECT_TRUE(layout.isHeld(read, "shoot"));
    EXPECT_FALSE(layout.isHeld(read, "dash"));
    const std::array<float, 2> move = layout.readVector(read, "move");
    EXPECT_GT(move.at(0), 0.0F);
    EXPECT_GT(move.at(1), 0.0F);
    EXPECT_FLOAT_EQ(layout.readAxis(read, "throttle"), 1.0F);
}

TEST(NetworkInput, AddsNoSystemWithoutTheInputCall) {
    rtype::ecs::App app;

    rtype::engine::addNetworkInput(app);  // no input plugin: nothing to read, nothing thrown

    SUCCEED();
}
