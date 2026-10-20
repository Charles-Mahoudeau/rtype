/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ClientPluginTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>

#include "ServerFixtures.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/client/NetClient.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/replication/ReplicationPlugin.hpp"
#include "rtype/replication/ReplicationSettings.hpp"
#include "rtype/replication/client/ClientPlugin.hpp"
#include "rtype/replication/client/ClientState.hpp"
#include "rtype/replication/component/NetworkId.hpp"
#include "rtype/replication/component/Owner.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/event/ClientConnected.hpp"
#include "rtype/replication/input/InputKind.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/server/ServerPlugin.hpp"
#include "rtype/replication/server/ServerState.hpp"

namespace {
namespace network = rtype::network;
using namespace std::chrono_literals;  // NOLINT(google-build-using-namespace): only the literals
using rtype::ecs::App;
using rtype::ecs::Commands;
using rtype::ecs::EventReader;
using rtype::ecs::FixedTimestep;
using rtype::ecs::Phase;
using rtype::ecs::Presence;
using rtype::ecs::Query;
using rtype::ecs::ReplicationMode;
using rtype::ecs::ResMut;
using rtype::ecs::RunMode;
using rtype::ecs::TimePlugin;
using rtype::replication::ReplicationPlugin;
using rtype::replication::ReplicationSettings;
using rtype::replication::client::ClientPlugin;
using rtype::replication::client::ClientState;
using rtype::replication::client::ConnectionStatus;
using rtype::replication::client::ManifestStatus;
using rtype::replication::component::NetworkId;
using rtype::replication::component::Owner;
using rtype::replication::component::PlayerInput;
using rtype::replication::event::ClientConnected;
using rtype::replication::input::InputKind;
using rtype::replication::input::InputLayout;
using rtype::replication::server::ServerPlugin;
using rtype::replication::server::ServerState;
using rtype::test::server::Position;
using rtype::test::server::Secret;

constexpr std::uint8_t kTickRate = 50;
constexpr std::chrono::milliseconds kFrame{20};

network::NetworkSettings makeNetworkSettings() {
    network::NetworkSettings settings;
    settings.port = 0;
    return settings;
}

/// @brief Game (server): spawns a ship for each client that joins.
void spawnShips(EventReader<ClientConnected> joined, Commands& commands) {
    for (const ClientConnected& event : joined) {
        commands.spawn().add(Owner{.client = event.client}).add(PlayerInput{}).add(Position{.x = 1.F, .y = 2.F});
    }
}

/// @brief Game (client): holds the fire button.
void pressFire(ResMut<ClientState> state) { state->setInput(PlayerInput{.buttons = 1}); }

/// @brief Game (server): moves the ships.
void moveShips(Query<Position> positions) {
    for (auto [entity, position] : positions) {
        position->x += 1.F;
    }
}

/// @brief Registers the game's components and input actions in an App.
void setUpGame(App& app, const bool withSecret = false) {
    app.addPlugin(TimePlugin::manual(kFrame, FixedTimestep{kTickRate, 5}));
    app.addPlugin(ReplicationPlugin{});
    auto& components = app.getWorld().getComponents();
    static_cast<void>(
        components.registerType<Position>({.presence = Presence::kBoth, .replication = ReplicationMode::kEveryChange}));
    if (withSecret) {
        static_cast<void>(components.registerType<Secret>(
            {.presence = Presence::kBoth, .replication = ReplicationMode::kEveryChange}));
    }
    auto layout = app.getWorld().getResources().getMut<InputLayout>();
    layout->add("fire", InputKind::kButton);
    layout->add("move", InputKind::kAxis);
}

/// @brief A server App, started, whose port a client connects to.
class Server {
  public:
    Server() : _app{std::make_unique<App>(RunMode::kServer)} {
        setUpGame(*_app);
        _app->addPlugin(ServerPlugin{makeNetworkSettings(), {.tickRate = kTickRate, .snapshotRate = kTickRate}});
        _app->addSystem(Phase::kFixedUpdate, "game.spawnShips", &spawnShips);
        _app->addSystem(Phase::kPostUpdate, "game.moveShips", &moveShips)
            .before(std::string{ServerPlugin::kSnapshotSystem});
        _app->runFrame();  // Startup: opens the socket
    }

    [[nodiscard]] App& app() { return *_app; }

    [[nodiscard]] ServerState& state() { return *_app->getWorld().getResources().getMut<ServerState>(); }

    [[nodiscard]] network::io::Address address() {
        return network::io::Address::resolve("127.0.0.1", state().netServer().localPort()).value();
    }

  private:
    std::unique_ptr<App> _app;
};

/// @brief A client App in a given mode, with the game's components and the plugin.
class Client {
  public:
    Client(const network::io::Address& server, const RunMode mode = RunMode::kClient, const bool withSecret = false)
        : _app{std::make_unique<App>(mode)} {
        setUpGame(*_app, withSecret);
        _app->addPlugin(ClientPlugin{server, makeNetworkSettings()});
    }

    [[nodiscard]] App& app() { return *_app; }

    [[nodiscard]] ClientState& state() { return *_app->getWorld().getResources().getMut<ClientState>(); }

  private:
    std::unique_ptr<App> _app;
};

/// @brief Runs frames of both apps until `done` or two seconds pass.
bool runUntil(Server& server, Client& client, const std::function<bool()>& done) {
    const auto deadline = network::Clock::now() + 2s;
    while (network::Clock::now() < deadline) {
        server.app().runFrame();
        client.app().runFrame();
        if (done()) {
            return true;
        }
        std::this_thread::sleep_for(1ms);
    }
    return false;
}
}  // namespace

TEST(ClientPlugin, IsNamed) {
    const auto address = network::io::Address::resolve("127.0.0.1", 9).value();
    EXPECT_EQ((ClientPlugin{address, network::NetworkSettings{}}.getName()), "rtype.replication.client");
    EXPECT_EQ(ClientPlugin::kName, "rtype.replication.client");
    EXPECT_EQ(ClientPlugin::kSendInputSystem, "net.sendInput");
}

TEST(ClientPlugin, AddsTheReplicationPluginAndTheState) {
    Server server;
    Client client{server.address()};
    EXPECT_TRUE(client.app().hasPlugin("rtype.replication"));
    EXPECT_TRUE(client.app().hasPlugin("rtype.replication.client"));
    EXPECT_TRUE(client.app().getWorld().getResources().has<ClientState>());
}

TEST(ClientPlugin, RunsOnlyInClientAndStandaloneModes) {
    Server server;
    for (const RunMode mode : {RunMode::kServer, RunMode::kClient, RunMode::kStandalone}) {
        Client client{server.address(), mode};
        client.app().runFrame();
        client.app().runFrame();
        const bool runs = mode != RunMode::kServer;
        EXPECT_EQ(client.state().connectionStatus() != ConnectionStatus::kIdle, runs);
    }
}

TEST(ClientPlugin, SendsReadyOnceTheManifestIsAccepted) {
    Server server;
    Client client{server.address()};
    ASSERT_TRUE(runUntil(server, client, [&] { return server.state().clientCount() == 1; }));
    EXPECT_EQ(client.state().manifestStatus(), ManifestStatus::kAccepted);
    EXPECT_EQ(client.state().connectionStatus(), ConnectionStatus::kConnected);
    EXPECT_EQ(client.state().serverSettings().tickRate, kTickRate);
}

TEST(ClientPlugin, DisconnectsAfterAMismatch) {
    Server server;
    Client client{server.address(), RunMode::kClient, true};
    ASSERT_TRUE(runUntil(server, client, [&] { return client.state().manifestStatus() != ManifestStatus::kWaiting; }));
    EXPECT_EQ(client.state().manifestStatus(), ManifestStatus::kMismatch);
    EXPECT_NE(client.state().mismatch().find("test.server.Secret"), std::string_view::npos);
    EXPECT_EQ(client.state().netClient().core().state(), network::client::ClientState::kClosed);
    for (int frame = 0; frame < 20; ++frame) {
        server.app().runFrame();
        std::this_thread::sleep_for(1ms);
    }
    EXPECT_EQ(server.state().clientCount(), 0U);
}

TEST(ClientPlugin, MirrorsTheServersEntitiesAfterThePreUpdatePhase) {
    Server server;
    Client client{server.address()};
    auto& world = client.app().getWorld();
    ASSERT_TRUE(runUntil(server, client, [&] { return client.state().lastAppliedTick() > 0; }));
    const auto player = client.state().localPlayer();
    ASSERT_TRUE(player);
    EXPECT_EQ(world.getComponents().get<Owner>(*player)->client, client.state().clientId().value());
    ASSERT_TRUE(runUntil(server, client, [&] { return world.getComponents().get<Position>(*player)->x > 3.F; }));
    EXPECT_EQ(world.getComponents().get<NetworkId>(*player)->value, 1U);
    EXPECT_EQ(client.state().find(1), *player);
}

TEST(ClientPlugin, SendsOneInputPerFrame) {
    Server server;
    Client client{server.address()};
    client.app()
        .addSystem(Phase::kPostUpdate, "game.pressFire", &pressFire)
        .setSide(rtype::ecs::Side::kClient)
        .before(std::string{ClientPlugin::kSendInputSystem});
    ASSERT_TRUE(runUntil(server, client, [&] { return server.state().clientCount() == 1; }));
    const network::ClientId id = client.state().clientId().value();
    ASSERT_TRUE(runUntil(server, client, [&] { return server.state().lastProcessedInput(id) >= 5; }));
    EXPECT_GT(server.state().lastAppliedSnapshot(id), 0U);
    const auto ship = server.state().findEntity(1);
    ASSERT_TRUE(ship);
    const std::uint32_t buttons = server.app().getWorld().getComponents().get<PlayerInput>(*ship)->buttons;
    EXPECT_EQ(buttons, 1U);
}
