/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptStandaloneTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/replication/standalone/StandalonePlugin.hpp"
#include "rtype/scripting/plugin/ScriptingPlugin.hpp"

namespace rtype::test::script_standalone {
/// @brief What a script reports: the test reads them, since a failing Luau assert only logs.
struct Checked {
    std::string what;
};
RTYPE_ECS_EVENT(Checked, "test.standalone.Checked", what);
}  // namespace rtype::test::script_standalone

namespace {
using rtype::ecs::App;
using rtype::ecs::RunMode;
using rtype::replication::standalone::StandalonePlugin;
using rtype::test::script_standalone::Checked;

constexpr std::chrono::milliseconds kFrame{20};
constexpr std::uint8_t kTickRate = 50;

/// @brief The scripts: packets of both directions, a server, a client and a both-sides system listening, each ordered
/// after the one before so that they share the Checked event without ambiguity.
constexpr std::string_view kScript = R"(
engine.net.packet("t.Up", { direction = "toServer", channel = "reliable", fields = { { "n", "u8" } } })
engine.net.packet("t.Down", { direction = "toClients", channel = "reliable", fields = { { "n", "u8" } } })

engine.ecs.system("t.Client", {
    phase = "Update", side = "client", query = {},
    calls = { "rtype.net.sendToServer", "rtype.replication.localClient" },
    events = { read = { "net.PacketReceived" }, send = { "test.standalone.Checked" } },
    run = function(ctx)
        for _, packet in engine.net.received(ctx) do
            ctx.events.send("test.standalone.Checked", { what = "client:" .. packet.name .. ":" .. packet.sender })
        end
        local me = ctx.call("rtype.replication.localClient", {})
        if not Sent then
            Sent = true
            ctx.events.send("test.standalone.Checked", { what = "local:" .. tostring(me.connected) .. ":" .. me.client .. ":" .. ctx.side })
            engine.net.sendToServer(ctx, "t.Up", { n = 7 })
        end
    end,
})

engine.ecs.system("t.Server", {
    phase = "Update", side = "server", query = {}, after = { "t.Client" },
    calls = { "rtype.net.sendToClients" },
    events = { read = { "net.PacketReceived", "net.ClientConnected" }, send = { "test.standalone.Checked" } },
    run = function(ctx)
        for joined in ctx.events.read("net.ClientConnected") do
            ctx.events.send("test.standalone.Checked", { what = "joined:" .. joined.client .. ":" .. ctx.side })
        end
        for _, packet in engine.net.received(ctx) do
            ctx.events.send("test.standalone.Checked", {
                what = "server:" .. packet.name .. ":" .. packet.sender .. ":" .. packet.values.n })
            local other = engine.net.sendToClients(ctx, "t.Down", { n = 9 }, 2)
            local mine = engine.net.sendToClients(ctx, "t.Down", { n = packet.values.n + 1 }, 1)
            ctx.events.send("test.standalone.Checked", { what = "sent:" .. tostring(other) .. ":" .. tostring(mine) })
        end
    end,
})

engine.ecs.system("t.Both", {
    phase = "Update", side = "both", query = {}, after = { "t.Server" },
    events = { read = { "net.PacketReceived" }, send = { "test.standalone.Checked" } },
    run = function(ctx)
        for _, packet in engine.net.received(ctx) do
            ctx.events.send("test.standalone.Checked", { what = "both:" .. packet.name .. ":" .. packet.sender })
        end
    end,
})

)";

std::string uniqueSuffix() {
    static std::atomic<unsigned> counter{0};
    return std::to_string(std::random_device{}()) + "-" + std::to_string(counter.fetch_add(1));
}

/// @brief A standalone app running a script, with the "fire" action declared and what the scripts reported.
class StandaloneWorld {
  public:
    explicit StandaloneWorld(const std::string_view source)
        : _directory{std::filesystem::temp_directory_path() / ("rtype-standalone-" + uniqueSuffix())} {
        std::filesystem::create_directories(_directory);
        std::ofstream{_directory / "main.luau"} << source;
        _app.getWorld().getEvents().registerType<Checked>();
        _app.addPlugin(rtype::ecs::TimePlugin::manual(kFrame, rtype::ecs::FixedTimestep{kTickRate, 5}));
        _app.addPlugin(StandalonePlugin{});
        _app.addPlugin(rtype::scripting::ScriptingPlugin{
            {.directory = _directory, .options = {.isClient = true, .isServer = true}}});
    }

    ~StandaloneWorld() {
        std::error_code error;
        std::filesystem::remove_all(_directory, error);
    }

    StandaloneWorld(const StandaloneWorld& other) = delete;
    StandaloneWorld& operator=(const StandaloneWorld& other) = delete;
    StandaloneWorld(StandaloneWorld&& other) = delete;
    StandaloneWorld& operator=(StandaloneWorld&& other) = delete;

    [[nodiscard]] App& app() noexcept { return _app; }

    /// @brief Runs a frame and returns what the scripts reported during it.
    std::vector<std::string> frame() {
        _app.runFrame();
        std::vector<std::string> checked;
        for (const Checked& check : _app.getWorld().getEvents().read<Checked>(_cursor)) {
            checked.push_back(check.what);
        }
        return checked;
    }

  private:
    std::filesystem::path _directory;  ///< The scripts, removed with the object.
    App _app{RunMode::kStandalone};
    rtype::ecs::EventCursor _cursor;
};

bool has(const std::vector<std::string>& list, const std::string& what) {
    return std::ranges::find(list, what) != list.end();
}
}  // namespace

TEST(ScriptStandalone, TheLocalPlayerJoinsAndIsClientOne) {
    StandaloneWorld world{kScript};

    const std::vector<std::string> first = world.frame();

    EXPECT_TRUE(has(first, "joined:1:server"));
    EXPECT_TRUE(has(first, "local:true:1:client"));
}

TEST(ScriptStandalone, PacketsLoopBackToTheSideTheyTravelTo) {
    StandaloneWorld world{kScript};
    world.frame();  // the client sends t.Up

    const std::vector<std::string> second = world.frame();  // delivered to the server, which answers
    EXPECT_TRUE(has(second, "server:t.Up:1:7"));
    EXPECT_TRUE(has(second, "sent:false:true"));  // client 2 does not exist
    EXPECT_FALSE(has(second, "client:t.Up:1"));   // a client does not see what goes to the server
    EXPECT_TRUE(has(second, "both:t.Up:1"));      // a system of both sides sees every packet

    const std::vector<std::string> third = world.frame();  // delivered to the client
    EXPECT_TRUE(has(third, "client:t.Down:0"));
    EXPECT_TRUE(has(third, "both:t.Down:0"));
    EXPECT_FALSE(std::ranges::any_of(third, [](const std::string& what) { return what.starts_with("server:t.Down"); }));
}

TEST(ScriptStandalone, ABadPacketIsAScriptErrorNotACrash) {
    StandaloneWorld world{R"(
engine.net.packet("t.Up", { direction = "toServer", channel = "reliable", fields = { { "n", "u8" } } })
engine.ecs.system("t.Bad", {
    phase = "Update", side = "client", query = {}, calls = { "rtype.net.sendToServer" },
    run = function(ctx)
        engine.net.sendToServer(ctx, "t.Up", { n = 300 })
        engine.net.sendToServer(ctx, "t.Unknown", {})
    end,
})
)"};

    testing::internal::CaptureStderr();
    EXPECT_NO_THROW(world.frame());
    EXPECT_NO_THROW(world.frame());
    const std::string errors = testing::internal::GetCapturedStderr();

    EXPECT_NE(errors.find("t.Bad"), std::string::npos);
}
