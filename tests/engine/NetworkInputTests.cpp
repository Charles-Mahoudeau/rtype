/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NetworkInputTests
*/

#include <gtest/gtest.h>

#include <array>

#include "plugins/input/Control.hpp"
#include "plugins/input/Input.hpp"
#include "plugins/input/InputAction.hpp"
#include "rtype/contract/platform/event/Event.hpp"
#include "rtype/contract/platform/input/Key.hpp"
#include "rtype/engine/NetworkInput.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/input/InputKind.hpp"
#include "rtype/replication/input/InputLayout.hpp"

namespace {
using rtype::contract::platform::input::Key;
using rtype::plugins::input::ActionType;
using rtype::plugins::input::Control;
using rtype::replication::input::InputKind;

/// @brief Presses @p key, as the backend plugin reports it.
void press(rtype::plugins::input::Input& input, const Key key) {
    input.handleEvent(rtype::contract::platform::Event{rtype::contract::platform::event::KeyPressed{.key = key}});
}
}  // namespace

TEST(NetworkInput, ReadsEachActionOfTheLayoutAsItsKind) {
    rtype::plugins::input::Input input;
    input.addAction("shoot", ActionType::kButton).bind(Control::key(Key::kSpace));
    input.addAction("move", ActionType::kVector2)
        .bindVector(Control::key(Key::kW), Control::key(Key::kS), Control::key(Key::kA), Control::key(Key::kD));
    input.addAction("throttle", ActionType::kAxis).bindAxis(Control::key(Key::kQ), Control::key(Key::kE));
    rtype::replication::input::InputLayout layout;
    layout.add("shoot", InputKind::kButton);
    layout.add("move", InputKind::kVector2);
    layout.add("throttle", InputKind::kAxis);
    layout.add("dash", InputKind::kButton);  // no action of the input plugin: stays neutral

    press(input, Key::kSpace);
    press(input, Key::kW);
    press(input, Key::kD);
    press(input, Key::kE);
    input.update();
    const rtype::replication::component::PlayerInput read = rtype::engine::readNetworkInput(input, layout);

    EXPECT_TRUE(layout.isHeld(read, "shoot"));
    EXPECT_FALSE(layout.isHeld(read, "dash"));
    const std::array<float, 2> move = layout.readVector(read, "move");
    EXPECT_GT(move.at(0), 0.0F);  // right
    EXPECT_GT(move.at(1), 0.0F);  // up
    EXPECT_FLOAT_EQ(layout.readAxis(read, "throttle"), 1.0F);
}
