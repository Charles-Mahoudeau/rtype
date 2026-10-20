/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ServerPluginTests
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
#include <variant>
#include <vector>

#include "ServerFixtures.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/exceptions/ScheduleException.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/Time.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/client/ClientEvent.hpp"
#include "rtype/network/client/NetClient.hpp"
#include "rtype/network/exceptions/NetworkExceptions.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/network/server/NetServer.hpp"
#include "rtype/replication/MessageType.hpp"
#include "rtype/replication/ReplicationPlugin.hpp"
#include "rtype/replication/ReplicationSettings.hpp"
#include "rtype/replication/component/NetworkId.hpp"
#include "rtype/replication/component/Owner.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/event/ClientConnected.hpp"
#include "rtype/replication/event/ClientDisconnected.hpp"
#include "rtype/replication/exceptions/ReplicationExceptions.hpp"
#include "rtype/replication/input/InputKind.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/input/InputMessage.hpp"
#include "rtype/replication/server/ServerPlugin.hpp"
#include "rtype/replication/server/ServerState.hpp"
#include "rtype/replication/snapshot/Snapshot.hpp"

namespace rtype::test::server_plugin {
/// @brief What the game systems of the tests record.
struct Log {
    std::uint32_t connected;     ///< ClientConnected events seen.
    std::uint32_t disconnected;  ///< ClientDisconnected events seen.
    std::uint32_t lastClient;    ///< The client of the last event.
    std::uint32_t buttons;       ///< The buttons a player's PlayerInput had when the game looked at it.
    std::uint32_t pending;       ///< Entities the spawner has to spawn.
};
RTYPE_ECS_RESOURCE(Log, "test.serverPlugin.Log", connected, disconnected, lastClient, buttons, pending);
}  // namespace rtype::test::server_plugin

namespace {
namespace snapshot = rtype::replication::snapshot;
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
using rtype::ecs::Res;
using rtype::ecs::ResMut;
using rtype::ecs::RunMode;
using rtype::ecs::TimePlugin;
using rtype::replication::ReplicationPlugin;
using rtype::replication::ReplicationSettings;
using rtype::replication::component::NetworkId;
using rtype::replication::component::Owner;
using rtype::replication::component::PlayerInput;
using rtype::replication::event::ClientConnected;
using rtype::replication::event::ClientDisconnected;
using rtype::replication::exceptions::ReplicationException;
using rtype::replication::input::InputKind;
using rtype::replication::input::InputLayout;
using rtype::replication::server::ServerPlugin;
using rtype::replication::server::ServerState;
using rtype::test::server::Follow;
using rtype::test::server::Position;
using rtype::test::server_plugin::Log;

/// One frame is one fixed tick: 50 Hz, 20 ms per frame.
constexpr std::uint8_t kTickRate = 50;
constexpr std::chrono::milliseconds kFrame{20};

network::NetworkSettings makeNetworkSettings() {
    network::NetworkSettings settings;
    settings.port = 0;
    return settings;
}

ReplicationSettings makeSettings(const std::uint8_t snapshotRate = kTickRate) {
    return {.tickRate = kTickRate, .snapshotRate = snapshotRate};
}

/// @brief Game: spawns a ship for each client that joins.
void spawnShips(EventReader<ClientConnected> joined, Commands& commands, ResMut<Log> log) {
    for (const ClientConnected& event : joined) {
        commands.spawn().add(Owner{.client = event.client}).add(PlayerInput{}).add(Position{});
        ++log->connected;
        log->lastClient = event.client;
    }
}

/// @brief Game: despawns nothing, only counts who left.
void countDepartures(EventReader<ClientDisconnected> left, ResMut<Log> log) {
    for (const ClientDisconnected& event : left) {
        ++log->disconnected;
        log->lastClient = event.client;
    }
}

/// @brief Game: spawns the entities the test asked for.
void spawnPending(Commands& commands, ResMut<Log> log) {
    for (; log->pending > 0; --log->pending) {
        commands.spawn().add(Position{});
    }
}

/// @brief Game: looks at the input of the players.
void lookAtInputs(Query<const PlayerInput> players, ResMut<Log> log) {
    for (auto [entity, input] : players) {
        log->buttons = input.buttons;
    }
}

/// @brief Game: moves players in PostUpdate (writes a replicated component).
void writePositions(Query<Position> positions) {
    for (auto [entity, position] : positions) {
        position->x += 1.F;
    }
}

/// @brief A server App in a given mode, with the game's components, a clock of one tick per frame, and the plugin.
class Server {
  public:
    explicit Server(const RunMode mode = RunMode::kServer, const ReplicationSettings& settings = makeSettings())
        : _app{std::make_unique<App>(mode)} {
        _app->addPlugin(TimePlugin::manual(kFrame, FixedTimestep{kTickRate, 5}));
        _app->addPlugin(ReplicationPlugin{});
        auto& components = _app->getWorld().getComponents();
        static_cast<void>(components.registerType<Position>(
            {.presence = Presence::kBoth, .replication = ReplicationMode::kEveryChange}));
        static_cast<void>(components.registerType<Follow>(
            {.presence = Presence::kBoth, .replication = ReplicationMode::kEveryChange}));
        auto layout = _app->getWorld().getResources().getMut<InputLayout>();
        layout->add("fire", InputKind::kButton);
        layout->add("move", InputKind::kAxis);
        _app->insertResource(Log{});
        _app->addPlugin(ServerPlugin{makeNetworkSettings(), settings});
    }

    [[nodiscard]] App& app() { return *_app; }

    [[nodiscard]] rtype::ecs::World& world() { return _app->getWorld(); }

    [[nodiscard]] ServerState& state() { return *_app->getWorld().getResources().getMut<ServerState>(); }

    [[nodiscard]] Log& log() { return *_app->getWorld().getResources().getMut<Log>(); }

    /// @return The layout the clients share.
    [[nodiscard]] const InputLayout& layout() { return *_app->getWorld().getResources().get<InputLayout>(); }

    /// @brief Adds the game systems that react to clients, after the plugin.
    void addGame() {
        _app->addSystem(Phase::kFixedUpdate, "game.1spawnShips", &spawnShips);
        _app->addSystem(Phase::kFixedUpdate, "game.2countDepartures", &countDepartures).after("game.1spawnShips");
        _app->addSystem(Phase::kFixedUpdate, "game.3spawnPending", &spawnPending).after("game.2countDepartures");
        _app->addSystem(Phase::kFixedUpdate, "game.4lookAtInputs", &lookAtInputs)
            .after(std::string{ServerPlugin::kApplyInputsSystem})
            .after("game.3spawnPending");
    }

  private:
    std::unique_ptr<App> _app;  ///< Not movable, so held by pointer.
};

/// @brief A client on loopback, driven by hand.
class Remote {
  public:
    explicit Remote(const std::uint16_t port)
        : _client{network::io::Address::resolve("127.0.0.1", port).value(), makeNetworkSettings()} {}

    /// @brief Polls and keeps the events.
    void poll() {
        _client.poll(network::Clock::now());
        for (network::client::ClientEvent& event : _client.takeEvents()) {
            if (auto* message = std::get_if<network::client::ServerMessage>(&event)) {
                _messages.push_back(std::move(message->payload));
            }
        }
    }

    [[nodiscard]] network::client::NetClient& client() { return _client; }

    [[nodiscard]] std::vector<network::Bytes>& messages() { return _messages; }

  private:
    network::client::NetClient _client;
    std::vector<network::Bytes> _messages;  ///< The messages of the server, in order.
};

/// @brief Runs frames, polling the remote client, until `done` or two seconds pass.
bool runUntil(Server& server, Remote* remote, const std::function<bool()>& done) {
    const auto deadline = network::Clock::now() + 2s;
    while (network::Clock::now() < deadline) {
        server.app().runFrame();
        if (remote != nullptr) {
            remote->poll();
        }
        if (done()) {
            return true;
        }
        std::this_thread::sleep_for(1ms);
    }
    return false;
}

/// @brief Connects a client: runs frames until the server knows it.
bool connect(Server& server, Remote& remote) {
    server.app().runFrame();  // Startup: opens the socket
    remote.client().connect(network::Clock::now());
    if (!runUntil(server, &remote,
                  [&] { return remote.client().core().state() == network::client::ClientState::kConnected; })) {
        return false;
    }
    remote.client().sendReady();
    return runUntil(server, &remote, [&] { return server.state().clientCount() == 1; });
}

/// @brief Opens the server and gives the port a client connects to.
std::uint16_t open(Server& server) {
    server.app().runFrame();
    return server.state().netServer().localPort();
}

/// @return The last snapshot a remote received, decoded.
std::optional<snapshot::Snapshot> lastSnapshot(Server& server, Remote& remote) {
    const std::vector<std::size_t> sizes = snapshot::computeComponentSizes(server.state().manifest(), server.world());
    for (auto message = remote.messages().rbegin(); message != remote.messages().rend(); ++message) {
        if (!message->empty() &&
            message->front() == static_cast<std::byte>(rtype::replication::MessageType::kSnapshot)) {
            return snapshot::decode(*message, sizes);
        }
    }
    return std::nullopt;
}

/// @return The NetworkId an entity was given, or 0.
std::uint32_t networkIdOf(Server& server, const rtype::ecs::Entity entity) {
    const NetworkId* id = server.world().getComponents().get<NetworkId>(entity);
    return id == nullptr ? 0 : id->value;
}
}  // namespace

TEST(ServerPlugin, IsNamed) {
    EXPECT_EQ((ServerPlugin{network::NetworkSettings{}, ReplicationSettings{}}.getName()), "rtype.replication.server");
    EXPECT_EQ(ServerPlugin::kName, "rtype.replication.server");
    EXPECT_EQ(ServerPlugin::kReceiveSystem, "net.receive");
    EXPECT_EQ(ServerPlugin::kApplyInputsSystem, "net.applyInputs");
    EXPECT_EQ(ServerPlugin::kAssignIdsSystem, "net.assignIds");
    EXPECT_EQ(ServerPlugin::kSnapshotSystem, "net.snapshot");
}

TEST(ServerPlugin, AddsTheReplicationPluginAndTheState) {
    Server server;
    EXPECT_TRUE(server.app().hasPlugin("rtype.replication"));
    EXPECT_TRUE(server.app().hasPlugin("rtype.replication.server"));
    EXPECT_TRUE(server.world().getResources().has<ServerState>());
}

TEST(ServerPlugin, AddingItTwiceDoesNothingTheSecondTime) {
    Server server;
    EXPECT_NO_THROW(server.app().addPlugin(ServerPlugin{makeNetworkSettings(), makeSettings()}));
}

TEST(ServerPlugin, OpensTheServerInServerMode) {
    Server server{RunMode::kServer};
    server.app().runFrame();
    EXPECT_TRUE(server.state().isStarted());
    EXPECT_NE(server.state().netServer().localPort(), 0U);
    EXPECT_TRUE(server.layout().isFrozen());
}

TEST(ServerPlugin, OpensTheServerInStandaloneMode) {
    Server server{RunMode::kStandalone};
    server.app().runFrame();
    EXPECT_TRUE(server.state().isStarted());
}

TEST(ServerPlugin, RunsNothingInClientMode) {
    Server server{RunMode::kClient};
    server.app().runFrame();
    server.app().runFrame();
    EXPECT_FALSE(server.state().isStarted());
    EXPECT_FALSE(server.layout().isFrozen());
}

TEST(ServerPlugin, StartFailsWhenTheSnapshotRateDoesNotDivideTheTickRate) {
    Server server{RunMode::kServer, makeSettings(7)};
    EXPECT_THROW(server.app().runFrame(), ReplicationException);
}

TEST(ServerPlugin, StartFailsWhenThePortIsTaken) {
    network::server::NetServer taken{makeNetworkSettings()};
    network::NetworkSettings settings;
    settings.port = taken.localPort();
    App app;
    app.addPlugin(TimePlugin::manual(kFrame, FixedTimestep{kTickRate, 5}));
    app.addPlugin(ServerPlugin{settings, makeSettings()});
    EXPECT_THROW(app.runFrame(), network::exceptions::SocketException);
}

TEST(ServerPlugin, StartNamesAReplicatedComponentRegisteredAfterThePlugin) {
    Server server;
    rtype::ecs::ComponentInfo info;
    info.type.name = "test.serverPlugin.Late";
    info.type.size = sizeof(float);
    info.type.alignment = alignof(float);
    info.type.presence = Presence::kBoth;
    info.type.replication = ReplicationMode::kEveryChange;
    info.type.fields.push_back({.name = "value", .type = rtype::ecs::FieldType::kF32, .offset = 0, .count = 1});
    ASSERT_TRUE(server.world().getComponents().registerType(info));

    try {
        server.app().runFrame();
        FAIL() << "net.start should refuse a component registered after the plugin";
    } catch (const ReplicationException& error) {
        EXPECT_NE(std::string{error.what()}.find("test.serverPlugin.Late"), std::string::npos) << error.what();
    }
}

TEST(ServerPlugin, AGameSystemWritingAReplicatedComponentMustBeOrderedAgainstTheSnapshot) {
    Server server;
    server.app().addSystem(Phase::kPostUpdate, "game.writePositions", &writePositions);
    try {
        server.app().runFrame();
        FAIL() << "the schedule should not build";
    } catch (const rtype::ecs::exceptions::ScheduleException& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find("game.writePositions"), std::string::npos) << message;
        EXPECT_NE(message.find("net.snapshot"), std::string::npos) << message;
        EXPECT_EQ(message.find("net.assignIds"), std::string::npos) << message;
    }
}

TEST(ServerPlugin, OrderingAWriterAgainstTheSnapshotIsEnough) {
    Server before;
    before.app()
        .addSystem(Phase::kPostUpdate, "game.writePositions", &writePositions)
        .before(std::string{ServerPlugin::kSnapshotSystem});
    EXPECT_NO_THROW(before.app().runFrame());

    Server after;
    after.app()
        .addSystem(Phase::kPostUpdate, "game.writePositions", &writePositions)
        .after(std::string{ServerPlugin::kSnapshotSystem});
    EXPECT_NO_THROW(after.app().runFrame());
}

TEST(ServerPlugin, GivesIncreasingIdsInTheFrameTheEntityWasSpawned) {
    Server server;
    server.addGame();
    server.app().runFrame();

    server.log().pending = 2;
    server.app().runFrame();

    std::vector<std::uint32_t> ids;
    for (std::uint32_t networkId = 1; networkId <= 2; ++networkId) {
        const auto entity = server.state().findEntity(networkId);
        ASSERT_TRUE(entity) << networkId;
        ids.push_back(networkIdOf(server, *entity));
    }
    EXPECT_EQ(ids, (std::vector<std::uint32_t>{1, 2}));
    EXPECT_EQ(server.state().entityCount(), 2U);
}

TEST(ServerPlugin, AnEntityWithSeveralReplicatedComponentsGetsOneId) {
    Server server;
    server.app().runFrame();
    const rtype::ecs::Entity target = server.world().spawn(Position{});
    const rtype::ecs::Entity both = server.world().spawn(Position{}, Follow{.target = target});

    server.app().runFrame();

    EXPECT_EQ(server.state().entityCount(), 2U);
    EXPECT_EQ(networkIdOf(server, target), 1U);
    EXPECT_EQ(networkIdOf(server, both), 2U);
    server.app().runFrame();
    EXPECT_EQ(server.state().entityCount(), 2U);
}

TEST(ServerPlugin, IgnoresEntitiesWithoutReplicatedComponents) {
    Server server;
    server.app().runFrame();
    static_cast<void>(server.world().spawn());
    server.app().runFrame();
    EXPECT_EQ(server.state().entityCount(), 0U);
}

TEST(ServerPlugin, NeverReusesAnId) {
    Server server;
    server.addGame();
    server.app().runFrame();
    server.log().pending = 1;
    server.app().runFrame();
    const rtype::ecs::Entity first = *server.state().findEntity(1);
    server.world().despawn(first);
    server.app().runFrame();  // forgets it, at the snapshot
    server.log().pending = 1;
    server.app().runFrame();

    EXPECT_FALSE(server.state().findEntity(1));
    EXPECT_TRUE(server.state().findEntity(2));
    EXPECT_EQ(server.state().entityCount(), 1U);
}

TEST(ServerPlugin, SendsSnapshotsEveryTickRateOverSnapshotRateTicks) {
    Server server{RunMode::kServer, makeSettings(25)};  // every 2 ticks
    std::vector<std::uint32_t> ticks;
    for (int frame = 0; frame < 8; ++frame) {
        server.app().runFrame();
        ticks.push_back(server.state().serverTick());
    }
    // One tick per frame, a snapshot every second tick.
    EXPECT_EQ(ticks, (std::vector<std::uint32_t>{0, 2, 2, 4, 4, 6, 6, 8}));
}

TEST(ServerPlugin, SendsASnapshotEveryFrameAtTheTickRate) {
    Server server;
    std::vector<std::uint32_t> ticks;
    for (int frame = 0; frame < 4; ++frame) {
        server.app().runFrame();
        ticks.push_back(server.state().serverTick());
    }
    EXPECT_EQ(ticks, (std::vector<std::uint32_t>{1, 2, 3, 4}));
}

TEST(ServerPlugin, AnnouncesClientsAndSendsThemSnapshots) {
    Server server;
    server.addGame();
    const std::uint16_t port = open(server);
    Remote remote{port};
    ASSERT_TRUE(connect(server, remote));

    ASSERT_TRUE(runUntil(server, &remote, [&] { return server.log().connected == 1; }));
    EXPECT_EQ(server.log().lastClient, 1U);
    EXPECT_EQ(server.log().disconnected, 0U);

    // The ship spawned by the game is in the next snapshots, with its owner.
    ASSERT_TRUE(runUntil(server, &remote, [&] {
        const std::optional<snapshot::Snapshot> last = lastSnapshot(server, remote);
        return last && !last->spawns.empty();
    }));
    const snapshot::Snapshot last = *lastSnapshot(server, remote);
    EXPECT_EQ(last.flags, snapshot::kComplete);
    ASSERT_EQ(last.spawns.size(), 1U);
    EXPECT_EQ(last.spawns[0].networkId, 1U);
    EXPECT_EQ(last.spawns[0].components.size(), 2U);  // Owner and Position
    EXPECT_EQ(last.updates.size(), 0U);
}

TEST(ServerPlugin, AnnouncesAClientThatLeaves) {
    Server server;
    server.addGame();
    const std::uint16_t port = open(server);
    Remote remote{port};
    ASSERT_TRUE(connect(server, remote));
    ASSERT_TRUE(runUntil(server, &remote, [&] { return server.log().connected == 1; }));

    remote.client().disconnect(network::Clock::now());

    ASSERT_TRUE(runUntil(server, &remote, [&] { return server.log().disconnected == 1; }));
    EXPECT_EQ(server.log().lastClient, 1U);
    EXPECT_EQ(server.state().clientCount(), 0U);
}

TEST(ServerPlugin, DeadEntitiesDisappearFromTheNextSnapshot) {
    Server server;
    server.addGame();
    const std::uint16_t port = open(server);
    Remote remote{port};
    ASSERT_TRUE(connect(server, remote));
    ASSERT_TRUE(runUntil(server, &remote, [&] {
        const std::optional<snapshot::Snapshot> last = lastSnapshot(server, remote);
        return last && !last->spawns.empty();
    }));

    server.world().despawn(*server.state().findEntity(1));
    const std::size_t received = remote.messages().size();

    ASSERT_TRUE(runUntil(server, &remote, [&] { return remote.messages().size() > received + 1; }));
    const snapshot::Snapshot last = *lastSnapshot(server, remote);
    EXPECT_TRUE(last.spawns.empty());
    EXPECT_EQ(last.flags, snapshot::kComplete);
    EXPECT_EQ(server.state().entityCount(), 0U);
}

TEST(ServerPlugin, AGameSystemOrderedAfterApplyInputsSeesTheInput) {
    Server server;
    server.addGame();
    const std::uint16_t port = open(server);
    Remote remote{port};
    ASSERT_TRUE(connect(server, remote));
    ASSERT_TRUE(runUntil(server, &remote, [&] { return server.log().connected == 1; }));
    ASSERT_TRUE(runUntil(server, &remote, [&] { return server.world().getEntityCount() == 1; }));
    ASSERT_EQ(server.log().buttons, 0U);

    const network::Bytes message = rtype::replication::input::encodeInput(
        {.lastAppliedSnapshot = 0, .inputs = {{.sequence = 1, .buttons = 0b1, .axes = {}}}}, server.layout());
    ASSERT_TRUE(remote.client().send(network::Channel::kUnreliable, message));

    ASSERT_TRUE(runUntil(server, &remote, [&] { return server.log().buttons == 0b1; }));
    EXPECT_EQ(server.state().lastProcessedInput(1), 1U);
}

TEST(ServerPlugin, TheSnapshotCarriesTheLastProcessedInputOfTheClient) {
    Server server;
    server.addGame();
    const std::uint16_t port = open(server);
    Remote remote{port};
    ASSERT_TRUE(connect(server, remote));
    ASSERT_TRUE(runUntil(server, &remote, [&] { return server.world().getEntityCount() == 1; }));

    const network::Bytes message = rtype::replication::input::encodeInput(
        {.lastAppliedSnapshot = 0, .inputs = {{.sequence = 3, .buttons = 0b1, .axes = {}}}}, server.layout());
    ASSERT_TRUE(remote.client().send(network::Channel::kUnreliable, message));

    ASSERT_TRUE(runUntil(server, &remote, [&] {
        const std::optional<snapshot::Snapshot> last = lastSnapshot(server, remote);
        return last && last->lastProcessedInput == 3;
    }));
}
