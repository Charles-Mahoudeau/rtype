/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputCallsTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstring>
#include <format>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/call/CallInfo.hpp"
#include "rtype/ecs/call/Calls.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/replication/ReplicationPlugin.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/exceptions/ReplicationExceptions.hpp"
#include "rtype/replication/input/InputCalls.hpp"
#include "rtype/replication/input/InputKind.hpp"
#include "rtype/replication/input/InputLayout.hpp"

namespace {
using rtype::replication::component::PlayerInput;
using rtype::replication::exceptions::InputLayoutException;
using rtype::replication::input::AddInputArguments;
using rtype::replication::input::InputKind;
using rtype::replication::input::InputLayout;
using rtype::replication::input::InputState;
using rtype::replication::input::ReadInputArguments;

/// @brief Makes the engine call @p name of @p app, as a script does, and returns its result.
/// @throws std::invalid_argument If no call has this name; whatever the call throws.
template <typename Result, typename Arguments>
Result call(rtype::ecs::App& app, const std::string_view name, const Arguments& arguments) {
    rtype::ecs::Calls& calls = app.getWorld().getCalls();
    const rtype::ecs::CallInfo* info = calls.findByName(name);
    if (info == nullptr) {
        throw std::invalid_argument{std::format("No engine call '{}'", name)};
    }
    std::array<std::byte, sizeof(Arguments)> bytes{};
    std::memcpy(bytes.data(), &arguments, sizeof(Arguments));
    if constexpr (std::is_void_v<Result>) {
        calls.invoke(app.getWorld(), info->id, bytes, std::span<std::byte>{});
    } else {
        std::array<std::byte, sizeof(Result)> result{};
        calls.invoke(app.getWorld(), info->id, bytes, result);
        Result value{};
        std::memcpy(&value, result.data(), sizeof(Result));
        return value;
    }
}

/// @brief A server App with the replication plugin, which registers the input calls.
struct InputCallsApp {
    rtype::ecs::App app{rtype::ecs::RunMode::kServer};

    InputCallsApp() { app.addPlugin(rtype::replication::ReplicationPlugin{}); }

    [[nodiscard]] const InputLayout& layout() const { return app.getWorld().getResources().require<InputLayout>(); }
};
}  // namespace

TEST(InputCalls, AddInputDeclaresAnActionOfEachKind) {
    InputCallsApp inputs;
    call<void>(inputs.app, rtype::replication::input::kAddInputCall, AddInputArguments{.name = "shoot"});
    call<void>(inputs.app, rtype::replication::input::kAddInputCall,
               AddInputArguments{.name = "move", .kind = "vector2"});
    call<void>(inputs.app, rtype::replication::input::kAddInputCall,
               AddInputArguments{.name = "throttle", .kind = "axis"});

    EXPECT_EQ(inputs.layout().find("shoot").kind, InputKind::kButton);  // a button by default
    EXPECT_EQ(inputs.layout().find("move").kind, InputKind::kVector2);
    EXPECT_EQ(inputs.layout().find("throttle").kind, InputKind::kAxis);
}

TEST(InputCalls, AddInputRefusesAnUnknownKindATakenNameAndAFrozenLayout) {
    InputCallsApp inputs;
    EXPECT_THROW(call<void>(inputs.app, rtype::replication::input::kAddInputCall,
                            AddInputArguments{.name = "move", .kind = "vector"}),
                 InputLayoutException);
    call<void>(inputs.app, rtype::replication::input::kAddInputCall, AddInputArguments{.name = "shoot"});
    EXPECT_THROW(call<void>(inputs.app, rtype::replication::input::kAddInputCall, AddInputArguments{.name = "shoot"}),
                 InputLayoutException);
    inputs.app.getWorld().getResources().requireMut<InputLayout>()->freeze();
    EXPECT_THROW(call<void>(inputs.app, rtype::replication::input::kAddInputCall, AddInputArguments{.name = "jump"}),
                 InputLayoutException);
}

TEST(InputCalls, ReadInputReadsAnActionOfAnEntitysPlayerInput) {
    InputCallsApp inputs;
    call<void>(inputs.app, rtype::replication::input::kAddInputCall, AddInputArguments{.name = "shoot"});
    call<void>(inputs.app, rtype::replication::input::kAddInputCall,
               AddInputArguments{.name = "move", .kind = "vector2"});
    call<void>(inputs.app, rtype::replication::input::kAddInputCall,
               AddInputArguments{.name = "throttle", .kind = "axis"});
    PlayerInput input;
    inputs.layout().setButton(input, "shoot", true);
    inputs.layout().setVector(input, "move", {0.5F, -1.0F});
    inputs.layout().setAxis(input, "throttle", 0.25F);
    const rtype::ecs::Entity ship = inputs.app.getWorld().spawn(input);

    const auto shoot = call<InputState>(inputs.app, rtype::replication::input::kReadInputCall,
                                        ReadInputArguments{.entity = ship, .action = "shoot"});
    EXPECT_TRUE(shoot.held);
    EXPECT_TRUE(shoot.pressed);  // not held at the input before
    EXPECT_FALSE(shoot.released);
    const auto move = call<InputState>(inputs.app, rtype::replication::input::kReadInputCall,
                                       ReadInputArguments{.entity = ship, .action = "move"});
    EXPECT_FALSE(move.held);  // the fields of another kind stay zero
    EXPECT_FLOAT_EQ(move.vector.x, 0.5F);
    EXPECT_FLOAT_EQ(move.vector.y, -1.0F);
    const auto throttle = call<InputState>(inputs.app, rtype::replication::input::kReadInputCall,
                                           ReadInputArguments{.entity = ship, .action = "throttle"});
    EXPECT_FLOAT_EQ(throttle.axis, 0.25F);
}

TEST(InputCalls, ReadInputRefusesAnUnknownActionAndAnEntityWithoutInput) {
    InputCallsApp inputs;
    call<void>(inputs.app, rtype::replication::input::kAddInputCall, AddInputArguments{.name = "shoot"});
    const rtype::ecs::Entity ship = inputs.app.getWorld().spawn(PlayerInput{});
    const rtype::ecs::Entity rock = inputs.app.getWorld().spawn();

    EXPECT_THROW(static_cast<void>(call<InputState>(inputs.app, rtype::replication::input::kReadInputCall,
                                                    ReadInputArguments{.entity = ship, .action = "jump"})),
                 InputLayoutException);
    EXPECT_THROW(static_cast<void>(call<InputState>(inputs.app, rtype::replication::input::kReadInputCall,
                                                    ReadInputArguments{.entity = rock, .action = "shoot"})),
                 InputLayoutException);
}
