/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** StandalonePluginTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/replication/component/Owner.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/event/ClientConnected.hpp"
#include "rtype/replication/input/InputKind.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/packet/PacketCalls.hpp"
#include "rtype/replication/packet/PacketSchema.hpp"
#include "rtype/replication/standalone/LocalSession.hpp"
#include "rtype/replication/standalone/StandalonePlugin.hpp"

namespace {
using rtype::ecs::App;
using rtype::ecs::RunMode;
using rtype::replication::component::Owner;
using rtype::replication::component::PlayerInput;
using rtype::replication::event::ClientConnected;
using rtype::replication::input::InputKind;
using rtype::replication::input::InputLayout;
using rtype::replication::standalone::LocalSession;
using rtype::replication::standalone::StandalonePlugin;

constexpr std::chrono::milliseconds kFrame{20};
constexpr std::uint8_t kTickRate = 50;  // one fixed step per frame

/// @return A standalone app with a manual clock, one step per frame, and a "fire" button in the layout.
std::unique_ptr<App> makeApp() {
    auto app = std::make_unique<App>(RunMode::kStandalone);
    app->addPlugin(rtype::ecs::TimePlugin::manual(kFrame, rtype::ecs::FixedTimestep{kTickRate, 5}));
    app->addPlugin(StandalonePlugin{});
    app->getWorld().getResources().getMut<InputLayout>()->add("fire", InputKind::kButton);
    return app;
}

/// @brief Gives the local player's input: "fire" held or not.
void hold(App& app, const bool held) {
    PlayerInput input;
    app.getWorld().getResources().get<InputLayout>()->setButton(input, "fire", held);
    app.getWorld().getResources().getMut<LocalSession>()->setInput(input);
}
}  // namespace

TEST(StandalonePlugin, IsNamedAndRegistersTheVocabulary) {
    const std::unique_ptr<App> holder = makeApp();
    App& app = *holder;

    EXPECT_EQ(StandalonePlugin{}.getName(), "rtype.replication.standalone");
    EXPECT_TRUE(app.hasPlugin("rtype.replication"));
    for (const char* call : {"rtype.net.declarePacket", "rtype.net.sendToServer", "rtype.net.sendToClients",
                             "rtype.replication.localClient", "rtype.replication.readInput"}) {
        EXPECT_NE(app.getWorld().getCalls().findByName(call), nullptr) << call;
    }
    EXPECT_NE(app.getWorld().getEvents().getRegistry().findByName("net.PacketReceived"), nullptr);
    EXPECT_NE(app.getWorld().getComponents().getRegistry().findByName("net.PlayerInput"), nullptr);
}

TEST(StandalonePlugin, RaisesClientConnectedOnceForClientOne) {
    const std::unique_ptr<App> holder = makeApp();
    App& app = *holder;
    rtype::ecs::EventCursor cursor;
    std::vector<std::uint32_t> clients;

    for (int frame = 0; frame < 4; ++frame) {
        app.runFrame();
        for (const ClientConnected& event : app.getWorld().getEvents().read<ClientConnected>(cursor)) {
            clients.push_back(event.client);
        }
    }

    ASSERT_EQ(clients.size(), 1U);
    EXPECT_EQ(clients.front(), 1U);
}

TEST(StandalonePlugin, GivesTheLocalInputToTheEntityOfClientOneWithEdges) {
    const std::unique_ptr<App> holder = makeApp();
    App& app = *holder;
    const rtype::ecs::Entity mine = app.getWorld().spawn(Owner{.client = 1}, PlayerInput{});
    const rtype::ecs::Entity other = app.getWorld().spawn(Owner{.client = 2}, PlayerInput{});
    const InputLayout& layout = *app.getWorld().getResources().get<InputLayout>();
    const auto fire = [&](const rtype::ecs::Entity entity) {
        return layout.isHeld(*app.getWorld().getComponents().get<PlayerInput>(entity), "fire");
    };
    const auto pressed = [&] {
        return layout.isPressed(*app.getWorld().getComponents().get<PlayerInput>(mine), "fire");
    };
    const auto released = [&] {
        return layout.isReleased(*app.getWorld().getComponents().get<PlayerInput>(mine), "fire");
    };

    hold(app, true);
    app.runFrame();
    EXPECT_TRUE(fire(mine));
    EXPECT_TRUE(pressed());
    EXPECT_FALSE(fire(other));

    app.runFrame();  // still held: no longer a press
    EXPECT_TRUE(fire(mine));
    EXPECT_FALSE(pressed());

    hold(app, false);
    app.runFrame();
    EXPECT_FALSE(fire(mine));
    EXPECT_TRUE(released());
}
