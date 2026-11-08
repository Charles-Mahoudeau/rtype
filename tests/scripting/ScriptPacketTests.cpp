/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptPacketTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/replication/MessageType.hpp"
#include "rtype/replication/ReplicationPlugin.hpp"
#include "rtype/replication/client/ClientPlugin.hpp"
#include "rtype/replication/client/ClientState.hpp"
#include "rtype/replication/exceptions/ReplicationExceptions.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/packet/PacketCalls.hpp"
#include "rtype/replication/packet/PacketCodec.hpp"
#include "rtype/replication/packet/PacketSchema.hpp"
#include "rtype/replication/server/ServerPlugin.hpp"
#include "rtype/replication/server/ServerState.hpp"
#include "rtype/scripting/plugin/ScriptingPlugin.hpp"

namespace rtype::test::script_packets {
/// @brief Sent by a script after a check of its own held: the test counts them, since a failing Luau assert only
/// stops the system and logs.
struct Checked {
    std::string what;
};
RTYPE_ECS_EVENT(Checked, "test.packets.Checked", what);
}  // namespace rtype::test::script_packets

namespace {
namespace network = rtype::network;
using namespace std::chrono_literals;  // NOLINT(google-build-using-namespace): only the literals
using rtype::ecs::App;
using rtype::ecs::RunMode;
using rtype::replication::client::ClientPlugin;
using rtype::replication::client::ClientState;
using rtype::replication::client::ManifestStatus;
using rtype::replication::packet::PacketReceived;
using rtype::replication::server::ServerPlugin;
using rtype::replication::server::ServerState;
using rtype::test::script_packets::Checked;

constexpr std::uint8_t kTickRate = 50;
constexpr std::chrono::milliseconds kFrame{20};

/// @brief The scripts both sides load: the packets (shared), the client's commands, the server's validation.
/// @details This is the documented example (docs/network/custom-packets.md).
constexpr std::string_view kShared = R"(
engine.net.packet("game.Buy", { direction = "toServer", channel = "reliable", fields = {
    { "item", "string", 32 }, { "qty", "u8" } } })
engine.net.packet("game.SetDirective", { direction = "toServer", channel = "reliable", fields = {
    { "ship", "u32" }, { "text", "string", 900 } } })
engine.net.packet("game.Ack", { direction = "toClients", channel = "reliable", fields = {
    { "ok", "bool" }, { "reason", "string", 32 } } })
)";

/// @brief Sends each command in order, retrying a refused one (not connected yet) on the next frame.
constexpr std::string_view kClient = R"(
local commands = {
    { "game.Buy", { item = "laser", qty = 2 } },
    { "game.Buy", { item = "nope", qty = 1 } },
    { "game.SetDirective", { ship = 7, text = string.rep("go north. ", 90) } },
}
local nextCommand = 1
engine.ecs.system("game.Commands", {
    phase = "Update", side = "client", query = {}, calls = { "rtype.net.sendToServer" },
    events = { send = { "test.packets.Checked" } },
    run = function(ctx)
        while nextCommand <= #commands do
            local command = commands[nextCommand]
            if not engine.net.sendToServer(ctx, command[1], command[2]) then
                break
            end
            ctx.events.send("test.packets.Checked", { what = "sent" })
            nextCommand += 1
        end
    end,
})
)";

constexpr std::string_view kServer = R"(
local catalog = { laser = 10 }
engine.ecs.system("game.Rules", {
    phase = "Update", side = "server", query = {}, calls = { "rtype.net.sendToClients" },
    events = { read = { "net.PacketReceived" }, send = { "test.packets.Checked" } },
    run = function(ctx)
        for _, packet in engine.net.received(ctx) do
            local reply
            if packet.name == "game.Buy" then
                local known = catalog[packet.values.item] ~= nil
                reply = { ok = known, reason = known and "" or "unknown_item" }
            elseif packet.name == "game.SetDirective" then
                reply = { ok = #packet.values.text == 900, reason = "directive" }
            end
            if engine.net.sendToClients(ctx, "game.Ack", reply, packet.sender) then
                ctx.events.send("test.packets.Checked", { what = "answered" })
            end
        end
    end,
})
)";

network::NetworkSettings makeNetworkSettings() {
    network::NetworkSettings settings;
    settings.port = 0;
    return settings;
}

/// @return A suffix that no other fixture or test process uses, so that two runs never share a directory.
std::string uniqueSuffix() {
    static std::atomic<unsigned> counter{0};
    return std::to_string(std::random_device{}()) + "-" + std::to_string(counter.fetch_add(1));
}

/// @brief A scripts directory in the temporary directory, removed with it.
class ScriptsDirectory {
  public:
    ScriptsDirectory(const std::string_view name, const std::string& content)
        : _path{std::filesystem::temp_directory_path() / (std::string{name} + "-" + uniqueSuffix())} {
        std::filesystem::create_directories(_path);
        std::ofstream{_path / "main.luau"} << content;
    }

    ~ScriptsDirectory() {
        std::error_code error;
        std::filesystem::remove_all(_path, error);
    }

    ScriptsDirectory(const ScriptsDirectory& other) = delete;
    ScriptsDirectory& operator=(const ScriptsDirectory& other) = delete;
    ScriptsDirectory(ScriptsDirectory&& other) = delete;
    ScriptsDirectory& operator=(ScriptsDirectory&& other) = delete;

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return _path; }

  private:
    std::filesystem::path _path;  ///< The directory.
};

/// @brief Declares packets from C++, as a plugin does; the scripts load after it.
using NativeDeclarations = std::function<void(rtype::replication::packet::PacketRegistry&)>;

std::unique_ptr<App> makeApp(const RunMode mode, const std::filesystem::path& scripts,
                             const NativeDeclarations& native) {
    auto app = std::make_unique<App>(mode);
    app->getWorld().getEvents().registerType<Checked>();
    app->addPlugin(rtype::ecs::TimePlugin::manual(kFrame, rtype::ecs::FixedTimestep{kTickRate, 5}));
    app->addPlugin(rtype::replication::ReplicationPlugin{});
    if (native) {
        native(*app->getWorld().getResources().getMut<rtype::replication::packet::PacketRegistry>());
    }
    app->addPlugin(rtype::scripting::ScriptingPlugin{
        {.directory = scripts,
         .options = {.isClient = mode == RunMode::kClient, .isServer = mode == RunMode::kServer}}});
    return app;
}

/// @brief A server and a client running the given scripts, connected on the loopback.
struct Pair {
    ScriptsDirectory serverScripts;
    ScriptsDirectory clientScripts;
    std::unique_ptr<App> server;
    std::unique_ptr<App> client;
    rtype::ecs::EventCursor serverCursor;
    rtype::ecs::EventCursor clientCursor;
    rtype::ecs::EventCursor serverChecks;
    rtype::ecs::EventCursor clientChecks;
    std::vector<std::string> serverChecked;      ///< What the server's scripts reported (Checked events).
    std::vector<std::string> clientChecked;      ///< What the client's scripts reported.
    std::vector<PacketReceived> serverReceived;  ///< Every packet the server's scripts were given.
    std::vector<PacketReceived> clientReceived;  ///< Every packet the client's scripts were given.

    Pair(const std::string_view tag, const std::string& serverSource, const std::string& clientSource,
         const std::uint8_t snapshotRate = kTickRate, const NativeDeclarations& native = {})
        : serverScripts{std::string{tag} + "-server", serverSource},
          clientScripts{std::string{tag} + "-client", clientSource},
          server{makeApp(RunMode::kServer, serverScripts.path(), native)},
          client{makeApp(RunMode::kClient, clientScripts.path(), native)} {
        server->addPlugin(ServerPlugin{makeNetworkSettings(), {.tickRate = kTickRate, .snapshotRate = snapshotRate}});
        server->runFrame();  // Startup: opens the socket
        const auto address = network::io::Address::resolve("127.0.0.1", state().netServer().localPort());
        client->addPlugin(ClientPlugin{address.value(), makeNetworkSettings()});
    }

    [[nodiscard]] ServerState& state() { return *server->getWorld().getResources().getMut<ServerState>(); }

    [[nodiscard]] ClientState& clientState() { return *client->getWorld().getResources().getMut<ClientState>(); }

    /// @brief Runs a frame of each app and collects the packets their scripts were given.
    void frame() {
        server->runFrame();
        client->runFrame();
        for (const PacketReceived& packet : server->getWorld().getEvents().read<PacketReceived>(serverCursor)) {
            serverReceived.push_back(packet);
        }
        for (const PacketReceived& packet : client->getWorld().getEvents().read<PacketReceived>(clientCursor)) {
            clientReceived.push_back(packet);
        }
        for (const Checked& check : server->getWorld().getEvents().read<Checked>(serverChecks)) {
            serverChecked.push_back(check.what);
        }
        for (const Checked& check : client->getWorld().getEvents().read<Checked>(clientChecks)) {
            clientChecked.push_back(check.what);
        }
    }

    /// @brief Runs frames until `done` or two seconds pass.
    bool runUntil(const std::function<bool()>& done) {
        const auto deadline = network::Clock::now() + 2s;
        while (network::Clock::now() < deadline) {
            frame();
            if (done()) {
                return true;
            }
            std::this_thread::sleep_for(1ms);
        }
        return false;
    }
};

std::string withShared(const std::string_view rest) { return std::string{kShared} + std::string{rest}; }
}  // namespace

TEST(ScriptPackets, AClientCommandIsValidatedByTheServerAndAnswered) {
    Pair pair{"rtype-packets-example", withShared(kServer), withShared(kClient)};

    ASSERT_TRUE(pair.runUntil([&] { return pair.clientReceived.size() >= 3; }));

    EXPECT_EQ(pair.clientState().manifestStatus(), ManifestStatus::kAccepted);
    ASSERT_EQ(pair.serverReceived.size(), 3U);
    EXPECT_EQ(pair.serverReceived[0].name, "game.Buy");
    EXPECT_EQ(pair.serverReceived[0].sender, pair.clientState().clientId().value());
    EXPECT_EQ(pair.serverReceived[0].values, "5:laser1:2");
    // The 900-byte directive travels whole in one message.
    EXPECT_EQ(pair.serverReceived[2].name, "game.SetDirective");
    EXPECT_GT(pair.serverReceived[2].values.size(), 900U);
    // The answers, in order: a known item, an unknown one, the directive.
    EXPECT_EQ(pair.clientReceived[0].values, "1:10:");
    EXPECT_EQ(pair.clientReceived[1].values, "1:012:unknown_item");
    EXPECT_EQ(pair.clientReceived[2].values, "1:19:directive");
    EXPECT_EQ(pair.clientReceived[0].sender, 0U);
    // Every send reported success, and the server's replies were all queued (checked in the scripts, counted here).
    EXPECT_EQ(std::ranges::count(pair.clientChecked, "sent"), 3);
    EXPECT_EQ(std::ranges::count(pair.serverChecked, "answered"), 3);
}

TEST(ScriptPackets, TheServerOnlySendsToTheClientItNames) {
    constexpr std::string_view kEcho = R"(
engine.ecs.system("game.Rules", {
    phase = "Update", side = "server", query = {}, calls = { "rtype.net.sendToClients" },
    events = { read = { "net.PacketReceived" }, send = { "test.packets.Checked" } },
    run = function(ctx)
        for _, packet in engine.net.received(ctx) do
            -- an id nobody has: nothing is sent, and the call says so
            if engine.net.sendToClients(ctx, "game.Ack", { ok = true, reason = "x" }, 4242) == false then
                ctx.events.send("test.packets.Checked", { what = "refused" })
            end
        end
    end,
})
)";
    Pair pair{"rtype-packets-unknown-client", withShared(kEcho), withShared(kClient)};

    ASSERT_TRUE(pair.runUntil([&] { return pair.serverReceived.size() >= 3; }));
    for (int frame = 0; frame < 20; ++frame) {
        pair.frame();
    }

    EXPECT_TRUE(pair.clientReceived.empty());
    EXPECT_EQ(std::ranges::count(pair.serverChecked, "refused"), 3);  // the assertion ran for each command
}

TEST(ScriptPackets, ASendThatBreaksTheSchemaIsAScriptErrorAndNothingTravels) {
    constexpr std::string_view kBad = R"(
local rejected = false
local sent = false
engine.ecs.system("game.Commands", {
    phase = "Update", side = "client", query = {}, calls = { "rtype.net.sendToServer" },
    events = { send = { "test.packets.Checked" } },
    run = function(ctx)
        if not rejected then
            rejected = true
            assert(not pcall(engine.net.sendToServer, ctx, "game.Buy", { item = "laser", qty = 256 }))
            assert(not pcall(engine.net.sendToServer, ctx, "game.Buy", { item = "laser", qty = 1.5 }))
            assert(not pcall(engine.net.sendToServer, ctx, "game.Buy", { item = string.rep("x", 33), qty = 1 }))
            assert(not pcall(engine.net.sendToServer, ctx, "game.Buy", { item = "laser" }))
            assert(not pcall(engine.net.sendToServer, ctx, "game.Ack", { ok = true, reason = "" }))
            assert(not pcall(engine.net.sendToServer, ctx, "game.Nope", {}))
            ctx.events.send("test.packets.Checked", { what = "rejected" })
        end
        if not sent and engine.net.sendToServer(ctx, "game.Buy", { item = "laser", qty = 255 }) then
            sent = true
            ctx.events.send("test.packets.Checked", { what = "sent" })
        end
    end,
})
)";
    Pair pair{"rtype-packets-bad-send", withShared(kServer), withShared(kBad)};

    ASSERT_TRUE(pair.runUntil([&] { return !pair.clientReceived.empty(); }));
    for (int frame = 0; frame < 20; ++frame) {
        pair.frame();
    }

    EXPECT_EQ(std::ranges::count(pair.clientChecked, "rejected"), 1);  // all six refusals held
    EXPECT_EQ(std::ranges::count(pair.clientChecked, "sent"), 1);
    ASSERT_EQ(pair.serverReceived.size(), 1U);  // only the valid one
    EXPECT_EQ(pair.serverReceived[0].values, "5:laser3:255");
}

TEST(ScriptPackets, ADeclarationThatWouldChangeTheSchemaIsRefusedWhenItLoads) {
    constexpr std::string_view kDeclarations = R"(
local function declare(name, fields)
    return pcall(engine.net.packet, name, { direction = "toServer", fields = fields })
end
local outcomes = {
    declare("game.Holes", { { "a", "u8" }, nil, { "b", "u8" } }),
    declare("game.Named", { { "a", "u8" }, extra = { "b", "u8" } }),
    declare("game.Fraction", { { "text", "string", 0.5 } }),
    declare("game.Zero", { { "text", "string", 0 } }),
    declare("game.Delimiter", { { "qty:u8\ncount", "u8" } }),
    declare("game.Fine", { { "a", "u8" }, { "text", "string", 12 } }),
    declare("game.Sparse", { { "a", "u8" }, [3] = { "b", "u8" }, [4] = { "c", "u8" } }),
    declare("game.SparseEnd", { [1] = { "a", "u8" }, [2] = { "b", "u8" }, [4] = { "c", "u8" } }),
}
engine.ecs.system("game.Report", {
    phase = "Update", side = "server", query = {},
    events = { send = { "test.packets.Checked" } },
    run = function(ctx)
        -- every frame: the first frame runs in the constructor of Pair, before anyone reads the events
        for index = 1, 8 do
            ctx.events.send("test.packets.Checked", { what = (outcomes[index] and "accepted" or "refused") .. index })
        end
    end,
})
)";
    Pair pair{"rtype-packets-declarations", std::string{kDeclarations}, withShared(kClient)};

    ASSERT_TRUE(pair.runUntil([&] { return pair.serverChecked.size() >= 8; }));

    const std::vector<std::string> expected{"refused1", "refused2",  "refused3", "refused4",
                                            "refused5", "accepted6", "refused7", "refused8"};
    EXPECT_EQ(std::vector<std::string>(pair.serverChecked.begin(), pair.serverChecked.begin() + 8), expected);
}

TEST(ScriptPackets, AScriptSendsAndReadsAPacketDeclaredInCpp) {
    constexpr std::string_view kNativeClient = R"(
local sent = false
engine.ecs.system("native.Send", {
    phase = "Update", side = "client", query = {}, calls = { "rtype.net.sendToServer" },
    run = function(ctx)
        if not sent and engine.net.sendToServer(ctx, "native.Ping", { n = 42, who = "me" }) then
            sent = true
        end
    end,
})
)";
    constexpr std::string_view kNativeServer = R"(
engine.ecs.system("native.Read", {
    phase = "Update", side = "server", query = {},
    events = { read = { "net.PacketReceived" }, send = { "test.packets.Checked" } },
    run = function(ctx)
        for _, packet in engine.net.received(ctx) do
            ctx.events.send("test.packets.Checked", { what = packet.name .. packet.values.n .. packet.values.who })
        end
    end,
})
)";
    Pair pair{
        "rtype-packets-native", std::string{kNativeServer}, std::string{kNativeClient}, kTickRate,
        [](rtype::replication::packet::PacketRegistry& registry) {
            using rtype::replication::packet::PacketField;
            using rtype::replication::packet::PacketType;
            registry.declare({.name = "native.Ping",
                              .direction = rtype::replication::packet::PacketDirection::kToServer,
                              .channel = network::Channel::kReliable,
                              .fields = {PacketField{.name = "n", .type = PacketType::kU8, .maxLength = 0},
                                         PacketField{.name = "who", .type = PacketType::kString, .maxLength = 8}}});
        }};

    ASSERT_TRUE(pair.runUntil([&] { return !pair.serverChecked.empty(); }));
    EXPECT_EQ(pair.serverChecked[0], "native.Ping42me");
}

TEST(ScriptPackets, AMalformedPacketIsRefusedBeforeItIsQueued) {
    Pair pair{"rtype-packets-malformed", withShared(kServer), withShared(kClient)};
    ASSERT_TRUE(pair.runUntil([&] { return pair.clientReceived.size() >= 3; }));
    const std::size_t before = pair.state().droppedPacketCount();

    // A custom packet whose body no schema accepts.
    const network::Bytes garbage{static_cast<std::byte>(rtype::replication::MessageType::kCustomPacket), std::byte{1},
                                 std::byte{2}, std::byte{3}};
    ASSERT_TRUE(pair.runUntil([&] { return pair.clientState().sendPacket(network::Channel::kReliable, garbage); }));
    ASSERT_TRUE(pair.runUntil([&] { return pair.state().droppedPacketCount() > before; }));

    EXPECT_EQ(pair.serverReceived.size(), 3U);  // nothing new reached the scripts
}

TEST(ScriptPackets, RefusesADeclarationAfterTheNetworkStarted) {
    Pair pair{"rtype-packets-late", withShared(kServer), withShared(kClient)};
    pair.frame();
    auto registry = pair.server->getWorld().getResources().getMut<rtype::replication::packet::PacketRegistry>();

    EXPECT_TRUE(registry->isFrozen());
    rtype::replication::packet::PacketSchema late{
        .name = "game.Late", .direction = {}, .channel = network::Channel::kReliable, .fields = {}};
    EXPECT_THROW(registry->declare(late), rtype::replication::exceptions::PacketException);
    // A reload declares the packets again, unchanged: accepted.
    EXPECT_NO_THROW(registry->declare(*registry->find("game.Buy")));
}

TEST(ScriptPackets, AnUnreliableCommandWaitsUntilTheServerAnswered) {
    // The transport hands a datagram's unreliable messages to the server before its reliable ones, so a command sent
    // in the datagram of `Ready` is dropped as "not ready". The client therefore refuses it until the server spoke.
    Pair pair{"rtype-packets-ready", withShared(kServer), withShared(kClient)};
    const auto& registry = *pair.client->getWorld().getResources().get<rtype::replication::packet::PacketRegistry>();
    const auto& schema = *registry.find("game.Buy");
    const auto message = rtype::replication::packet::encodePacket(schema, {std::string{"laser"}, 1.0});
    ASSERT_TRUE(pair.runUntil([&] { return pair.clientState().manifestStatus() == ManifestStatus::kAccepted; }));

    // The manifest is accepted, `Ready` is on its way and nothing came back yet.
    EXPECT_FALSE(pair.clientState().sendPacket(network::Channel::kUnreliable, message));
    EXPECT_TRUE(pair.clientState().sendPacket(network::Channel::kReliable, message));  // ordered after `Ready`

    // Once the server's snapshots arrive, unreliable commands go.
    ASSERT_TRUE(pair.runUntil([&] { return pair.clientState().sendPacket(network::Channel::kUnreliable, message); }));
    // `game.Buy` is declared reliable: the reliable one reaches the server's scripts, the one that came on the
    // unreliable channel is refused and counted (it could be duplicated or reordered).
    const std::size_t before = pair.state().droppedPacketCount();
    EXPECT_TRUE(pair.runUntil([&] {
        return std::ranges::count(pair.serverReceived, "5:laser1:1", &PacketReceived::values) >= 1 &&
               pair.state().droppedPacketCount() > before;
    }));
    EXPECT_EQ(std::ranges::count(pair.serverReceived, "5:laser1:1", &PacketReceived::values), 1);
}

TEST(ScriptPackets, TheServerPushesASnapshotAtTenHertzWhileCommandsFlow) {
    Pair pair{"rtype-packets-rate", withShared(kServer), withShared(kClient), 10};
    std::set<std::uint32_t> ticks;
    std::set<std::uint32_t> applied;

    for (int frame = 0; frame < 100; ++frame) {
        pair.frame();
        ticks.insert(pair.state().serverTick());
        applied.insert(pair.clientState().lastAppliedTick());
        std::this_thread::sleep_for(1ms);  // the datagrams cross the loopback between two frames
    }

    // 100 frames at 50 Hz are two seconds: a snapshot every 5 ticks is 10 per second.
    EXPECT_GE(ticks.size() - 1, 18U);
    EXPECT_LE(ticks.size() - 1, 21U);
    // The client really applied them (loopback loses none), and the commands went through meanwhile.
    EXPECT_GE(applied.size(), 10U);
    EXPECT_TRUE(pair.runUntil([&] { return pair.serverReceived.size() >= 3; }));
    EXPECT_EQ(std::ranges::count(pair.clientChecked, "sent"), 3);
}
