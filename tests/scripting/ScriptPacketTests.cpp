/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptPacketTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/replication/ReplicationPlugin.hpp"
#include "rtype/replication/client/ClientPlugin.hpp"
#include "rtype/replication/client/ClientState.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/packet/PacketCalls.hpp"
#include "rtype/replication/server/ServerPlugin.hpp"
#include "rtype/replication/server/ServerState.hpp"
#include "rtype/scripting/plugin/ScriptingPlugin.hpp"

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

constexpr std::string_view kClient = R"(
local sent = 0
engine.ecs.system("game.Commands", {
    phase = "Update", side = "client", query = {}, calls = { "rtype.net.sendToServer" },
    run = function(ctx)
        sent += 1
        if sent == 3 then
            engine.net.sendToServer(ctx, "game.Buy", { item = "laser", qty = 2 })
            engine.net.sendToServer(ctx, "game.Buy", { item = "nope", qty = 1 })
            engine.net.sendToServer(ctx, "game.SetDirective", { ship = 7, text = string.rep("go north. ", 90) })
        end
    end,
})
)";

constexpr std::string_view kServer = R"(
local catalog = { laser = 10 }
engine.ecs.system("game.Rules", {
    phase = "Update", side = "server", query = {}, calls = { "rtype.net.sendToClients" },
    events = { read = { "net.PacketReceived" } },
    run = function(ctx)
        for _, packet in engine.net.received(ctx) do
            if packet.name == "game.Buy" then
                local known = catalog[packet.values.item] ~= nil
                engine.net.sendToClients(ctx, "game.Ack", { ok = known, reason = known and "" or "unknown_item" }, packet.sender)
            elseif packet.name == "game.SetDirective" then
                engine.net.sendToClients(ctx, "game.Ack", { ok = #packet.values.text == 900, reason = "directive" }, packet.sender)
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

/// @brief A scripts directory in the temporary directory, removed with it.
class ScriptsDirectory {
  public:
    ScriptsDirectory(const std::string_view name, const std::string& content)
        : _path{std::filesystem::temp_directory_path() / std::string{name}} {
        std::filesystem::remove_all(_path);
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

std::unique_ptr<App> makeApp(const RunMode mode, const std::filesystem::path& scripts) {
    auto app = std::make_unique<App>(mode);
    app->addPlugin(rtype::ecs::TimePlugin::manual(kFrame, rtype::ecs::FixedTimestep{kTickRate, 5}));
    app->addPlugin(rtype::replication::ReplicationPlugin{});
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
    std::vector<PacketReceived> serverReceived;  ///< Every packet the server's scripts were given.
    std::vector<PacketReceived> clientReceived;  ///< Every packet the client's scripts were given.

    Pair(const std::string_view tag, const std::string& serverSource, const std::string& clientSource,
         const std::uint8_t snapshotRate = kTickRate)
        : serverScripts{std::string{tag} + "-server", serverSource},
          clientScripts{std::string{tag} + "-client", clientSource},
          server{makeApp(RunMode::kServer, serverScripts.path())},
          client{makeApp(RunMode::kClient, clientScripts.path())} {
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
}

TEST(ScriptPackets, TheServerOnlySendsToTheClientItNames) {
    constexpr std::string_view kEcho = R"(
engine.ecs.system("game.Rules", {
    phase = "Update", side = "server", query = {}, calls = { "rtype.net.sendToClients" },
    events = { read = { "net.PacketReceived" } },
    run = function(ctx)
        for _, packet in engine.net.received(ctx) do
            -- an id nobody has: nothing is sent
            assert(engine.net.sendToClients(ctx, "game.Ack", { ok = true, reason = "x" }, 4242) == false)
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
}

TEST(ScriptPackets, ASendThatBreaksTheSchemaIsAScriptErrorAndNothingTravels) {
    constexpr std::string_view kBad = R"(
local sent = 0
engine.ecs.system("game.Commands", {
    phase = "Update", side = "client", query = {}, calls = { "rtype.net.sendToServer" },
    run = function(ctx)
        sent += 1
        if sent == 3 then
            assert(not pcall(engine.net.sendToServer, ctx, "game.Buy", { item = "laser", qty = 256 }))
            assert(not pcall(engine.net.sendToServer, ctx, "game.Buy", { item = "laser", qty = 1.5 }))
            assert(not pcall(engine.net.sendToServer, ctx, "game.Buy", { item = string.rep("x", 33), qty = 1 }))
            assert(not pcall(engine.net.sendToServer, ctx, "game.Buy", { item = "laser" }))
            assert(not pcall(engine.net.sendToServer, ctx, "game.Ack", { ok = true, reason = "" }))
            assert(not pcall(engine.net.sendToServer, ctx, "game.Nope", {}))
            engine.net.sendToServer(ctx, "game.Buy", { item = "laser", qty = 255 })
        end
    end,
})
)";
    Pair pair{"rtype-packets-bad-send", withShared(kServer), withShared(kBad)};

    ASSERT_TRUE(pair.runUntil([&] { return !pair.clientReceived.empty(); }));
    for (int frame = 0; frame < 20; ++frame) {
        pair.frame();
    }

    ASSERT_EQ(pair.serverReceived.size(), 1U);  // only the valid one
    EXPECT_EQ(pair.serverReceived[0].values, "5:laser3:255");
}

TEST(ScriptPackets, RefusesADeclarationAfterTheNetworkStarted) {
    Pair pair{"rtype-packets-late", withShared(kServer), withShared(kClient)};
    pair.frame();

    EXPECT_TRUE(pair.server->getWorld().getResources().get<rtype::replication::packet::PacketRegistry>()->isFrozen());
}

TEST(ScriptPackets, TheServerPushesASnapshotAtTenHertzWhileCommandsFlow) {
    Pair pair{"rtype-packets-rate", withShared(kServer), withShared(kClient), 10};
    std::set<std::uint32_t> ticks;

    for (int frame = 0; frame < 100; ++frame) {
        pair.frame();
        ticks.insert(pair.state().serverTick());
    }

    // 100 frames at 50 Hz are two seconds: a snapshot every 5 ticks is 10 per second.
    EXPECT_GE(ticks.size() - 1, 18U);
    EXPECT_LE(ticks.size() - 1, 21U);
}
