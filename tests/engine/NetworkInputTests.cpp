/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NetworkInputTests
*/

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <glm/ext/vector_float2.hpp>
#include <optional>
#include <string_view>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/call/CallTraits.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/ecs/type/String.hpp"
#include "rtype/engine/NetworkInput.hpp"
#include "rtype/plugins/GlmReflection.hpp"
#include "rtype/replication/component/Owner.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/input/InputKind.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/standalone/StandalonePlugin.hpp"

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

namespace {
namespace fake_input {
/// @brief The arguments and the result of `rtype.input.read`, named and laid out as the Engine reads them.
struct ReadActionArguments {
    rtype::ecs::String action;
};
RTYPE_ECS_CALL_DATA(ReadActionArguments, "rtype.input.ReadActionArguments", action);

struct ActionState {
    glm::vec2 value{0.0F};
    bool held{false};
    bool pressed{false};
    bool released{false};
};
RTYPE_ECS_CALL_DATA(ActionState, "rtype.input.ActionState", value, held, pressed, released);

ActionState read(const ReadActionArguments& /*arguments*/) { return ActionState{.value = {1.0F, 0.0F}, .held = true}; }
}  // namespace fake_input
}  // namespace

TEST(NetworkInput, AStandaloneWorldGivesTheInputToTheEntityOfTheLocalClient) {
    rtype::ecs::App app{rtype::ecs::RunMode::kStandalone};
    app.addPlugin(rtype::ecs::TimePlugin::manual(std::chrono::milliseconds{20}, rtype::ecs::FixedTimestep{50, 5}));
    app.addCall("rtype.input.read", &fake_input::read);
    app.addPlugin(rtype::replication::standalone::StandalonePlugin{});
    app.getWorld().getResources().getMut<rtype::replication::input::InputLayout>()->add("fire", InputKind::kButton);
    rtype::engine::addNetworkInput(app);
    const rtype::ecs::Entity ship = app.getWorld().spawn(rtype::replication::component::Owner{.client = 1},
                                                         rtype::replication::component::PlayerInput{});

    app.runFrame();  // PostUpdate reads the player's input
    app.runFrame();  // the FixedUpdate hands it to the ship

    const auto& layout = *app.getWorld().getResources().get<rtype::replication::input::InputLayout>();
    EXPECT_TRUE(
        layout.isHeld(*app.getWorld().getComponents().get<rtype::replication::component::PlayerInput>(ship), "fire"));
}
