/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptContextTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/scripting/ScriptHost.hpp"

namespace rtype::test::script_context {
/// @brief A plain-data resource scripts read and write.
struct Score {
    float value;
    std::int32_t count;
};
RTYPE_ECS_RESOURCE(Score, "test.ctx.Score", value, count);

/// @brief A resource that owns memory: scripts cannot be given its bytes.
struct Bag {
    std::vector<int> items;
};
RTYPE_ECS_RESOURCE(Bag, "test.ctx.Bag");

/// @brief A plain-data resource that never gets a value.
struct Empty {
    float value;
};
RTYPE_ECS_RESOURCE(Empty, "test.ctx.Empty", value);

/// @brief A plain-data event.
struct Boom {
    std::int32_t power;
    float size{2.F};
};
RTYPE_ECS_EVENT(Boom, "test.ctx.Boom", power, size);

/// @brief An event that owns memory.
struct Crowd {
    std::vector<int> members;
};
RTYPE_ECS_EVENT(Crowd, "test.ctx.Crowd");
}  // namespace rtype::test::script_context

namespace {
using rtype::test::script_context::Boom;
using rtype::test::script_context::Score;

/// @brief An App with a manual clock, the C++ resources and events scripts use, and a host on it.
struct Fixture {
    rtype::ecs::App app;
    std::unique_ptr<rtype::scripting::ScriptHost> host;

    Fixture() {
        app.addPlugin(rtype::ecs::TimePlugin::manual(std::chrono::seconds{1}, rtype::ecs::FixedTimestep{1, 5}));
        app.getWorld().getResources().insert(Score{.value = 0.F, .count = 0});
        app.getWorld().getResources().registerType<rtype::test::script_context::Empty>();
        app.getWorld().getResources().insert(rtype::test::script_context::Bag{});
        app.getWorld().getEvents().registerType<Boom>();
        app.getWorld().getEvents().registerType<rtype::test::script_context::Crowd>();
        auto created = rtype::scripting::ScriptHost::create(app, {});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("context-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }

    [[nodiscard]] const Score& score() const { return app.getWorld().getResources().require<Score>(); }
};

}  // namespace

TEST(ScriptContext, AScriptReadsTheTimeResource) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.ReadTime", {
            phase = "Update",
            query = {},
            resources = { read = { "rtype.Time" }, write = { "test.ctx.Score" } },
            run = function(ctx)
                local time = ctx.resources["rtype.Time"]
                ctx.resources["test.ctx.Score"].value = time.delta + time.fixedDelta
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_FLOAT_EQ(fixture.score().value, 1.F + 1.F);  // the frame lasts one second, a fixed tick as well
}

TEST(ScriptContext, WritingAResourceDeclaredReadRaisesAnError) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.WriteTime", {
            phase = "Update",
            query = {},
            resources = { read = { "rtype.Time" }, write = { "test.ctx.Score" } },
            run = function(ctx)
                local ok, message = pcall(function() ctx.resources["rtype.Time"].delta = 5 end)
                assert(not ok and string.find(message, "read%-only"), message)
                ctx.resources["test.ctx.Score"].count = 1
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.score().count, 1);
}

TEST(ScriptContext, AWrittenResourceIsChangedInTheWorld) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Bump", {
            phase = "Update",
            query = {},
            resources = { write = { "test.ctx.Score" } },
            run = function(ctx)
                local score = ctx.resources["test.ctx.Score"]
                score.count = score.count + 1
            end,
        })
    )"),
              "");

    fixture.app.runFrame();
    fixture.app.runFrame();

    EXPECT_EQ(fixture.score().count, 2);
}

TEST(ScriptContext, ADeclaredResourceWithoutValueIsNil) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Missing", {
            phase = "Update",
            query = {},
            resources = { read = { "test.ctx.Empty" }, write = { "test.ctx.Score" } },
            run = function(ctx)
                assert(ctx.resources["test.ctx.Empty"] == nil)
                ctx.resources["test.ctx.Score"].count = 3
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.score().count, 3);
}

TEST(ScriptContext, AResourceThatWasNotDeclaredRaisesAnError) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Undeclared", {
            phase = "Update",
            query = {},
            resources = { write = { "test.ctx.Score" } },
            run = function(ctx)
                local ok, message = pcall(function() return ctx.resources["rtype.Time"] end)
                assert(not ok and string.find(message, "not declared"), message)
                ctx.resources["test.ctx.Score"].count = 1
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.score().count, 1);
}

TEST(ScriptContext, ResourcesOfASystemThatReturnedAreNoLongerValid) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        saved = nil
        engine.ecs.system("game.Keep", {
            phase = "Update",
            query = {},
            resources = { write = { "test.ctx.Score" } },
            run = function(ctx)
                if saved == nil then
                    saved = ctx.resources
                else
                    local ok, message = pcall(function() return saved["test.ctx.Score"] end)
                    assert(not ok and string.find(message, "no longer valid"), message)
                    ctx.resources["test.ctx.Score"].count = 1
                end
            end,
        })
    )"),
              "");

    fixture.app.runFrame();
    fixture.app.runFrame();

    EXPECT_EQ(fixture.score().count, 1);
}

TEST(ScriptContext, AResourceThatIsNotPlainDataIsRefusedWhenTheScriptLoads) {
    Fixture fixture;
    const std::string message = fixture.run(R"(
        engine.ecs.system("game.Bag", {
            phase = "Update",
            query = {},
            resources = { read = { "test.ctx.Bag" } },
            run = function(ctx) end,
        })
    )");
    EXPECT_NE(message.find("System 'game.Bag': resource 'test.ctx.Bag' cannot be used by a script"), std::string::npos)
        << message;
}

TEST(ScriptContext, AnUnknownResourceOrEventIsRefusedWhenTheScriptLoads) {
    Fixture fixture;
    const std::string resource = fixture.run(R"(
        engine.ecs.system("game.A", { phase = "Update", query = {}, resources = { read = { "game.Nope" } },
                                      run = function(ctx) end })
    )");
    EXPECT_NE(resource.find("System 'game.A': unknown resource 'game.Nope'"), std::string::npos) << resource;
    const std::string event = fixture.run(R"(
        engine.ecs.system("game.B", { phase = "Update", query = {}, events = { send = { "game.Nope" } },
                                      run = function(ctx) end })
    )");
    EXPECT_NE(event.find("System 'game.B': unknown event 'game.Nope'"), std::string::npos) << event;
}

TEST(ScriptContext, AnEventThatIsNotPlainDataIsRefusedWhenTheScriptLoads) {
    Fixture fixture;
    const std::string message = fixture.run(R"(
        engine.ecs.system("game.Crowd", { phase = "Update", query = {}, events = { read = { "test.ctx.Crowd" } },
                                          run = function(ctx) end })
    )");
    EXPECT_NE(message.find("event 'test.ctx.Crowd' cannot be used by a script"), std::string::npos) << message;
}

TEST(ScriptContext, BadResourceAndEventDeclarationsNameTheSystem) {
    Fixture fixture;
    EXPECT_NE(fixture
                  .run(R"(
        engine.ecs.system("game.C", { phase = "Update", query = {}, resources = { send = { "rtype.Time" } },
                                      run = function(ctx) end })
    )")
                  .find("System 'game.C': unknown key 'send' in 'resources' (expected 'read' or 'write')"),
              std::string::npos);
    EXPECT_NE(fixture
                  .run(R"(
        engine.ecs.system("game.D", { phase = "Update", query = {}, events = { write = { "test.ctx.Boom" } },
                                      run = function(ctx) end })
    )")
                  .find("System 'game.D': unknown key 'write' in 'events' (expected 'read' or 'send')"),
              std::string::npos);
    EXPECT_NE(fixture
                  .run(R"(
        engine.ecs.system("game.E", { phase = "Update", query = {}, events = { read = "test.ctx.Boom" },
                                      run = function(ctx) end })
    )")
                  .find("System 'game.E': 'events.read' must be a list of strings"),
              std::string::npos);
}

TEST(ScriptContext, AScriptSendsAnEventACppSystemReads) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Explode", {
            phase = "Update",
            query = {},
            events = { send = { "test.ctx.Boom" } },
            run = function(ctx)
                ctx.events.send("test.ctx.Boom", { power = 9 })
                ctx.events.send("test.ctx.Boom")
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    rtype::ecs::EventCursor cursor;
    const auto booms = fixture.app.getWorld().getEvents().read<Boom>(cursor);
    ASSERT_EQ(booms.size(), 2U);
    auto boom = booms.begin();
    EXPECT_EQ((*boom).power, 9);
    EXPECT_FLOAT_EQ((*boom).size, 2.F);  // the default the C++ type declares
    ++boom;
    EXPECT_EQ((*boom).power, 0);
}

TEST(ScriptContext, AScriptReadsEventsACppSystemSent) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Listen", {
            phase = "Update",
            query = {},
            events = { read = { "test.ctx.Boom" } },
            resources = { write = { "test.ctx.Score" } },
            run = function(ctx)
                local score = ctx.resources["test.ctx.Score"]
                for boom in ctx.events.read("test.ctx.Boom") do
                    score.count = score.count + boom.power
                    score.value = boom.size
                end
            end,
        })
    )"),
              "");
    fixture.app.getWorld().getEvents().send(Boom{.power = 3, .size = 5.F});
    fixture.app.getWorld().getEvents().send(Boom{.power = 4, .size = 6.F});

    fixture.app.runFrame();
    EXPECT_EQ(fixture.score().count, 7);
    EXPECT_FLOAT_EQ(fixture.score().value, 6.F);

    fixture.app.runFrame();
    EXPECT_EQ(fixture.score().count, 7);  // each event is read once
}

TEST(ScriptContext, TheViewOfAnEventRefusesWrites) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Listen", {
            phase = "Update",
            query = {},
            events = { read = { "test.ctx.Boom" } },
            resources = { write = { "test.ctx.Score" } },
            run = function(ctx)
                for boom in ctx.events.read("test.ctx.Boom") do
                    local ok, message = pcall(function() boom.power = 1 end)
                    assert(not ok and string.find(message, "read%-only"), message)
                    ctx.resources["test.ctx.Score"].count = 1
                end
            end,
        })
    )"),
              "");
    fixture.app.getWorld().getEvents().send(Boom{.power = 3, .size = 5.F});

    fixture.app.runFrame();

    EXPECT_EQ(fixture.score().count, 1);
}

TEST(ScriptContext, EventsAreCheckedAgainstWhatTheSystemDeclared) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Strict", {
            phase = "Update",
            query = {},
            events = { read = { "test.ctx.Boom" } },
            resources = { write = { "test.ctx.Score" } },
            run = function(ctx)
                local function fails(pattern, f)
                    local ok, message = pcall(f)
                    assert(not ok and string.find(message, pattern), tostring(message))
                end
                fails("not sent", function() ctx.events.send("test.ctx.Boom", {}) end)
                fails("not declared", function() ctx.events.read("test.ctx.Crowd") end)
                fails("expects the name", function() ctx.events.send() end)
                ctx.resources["test.ctx.Score"].count = 1
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.score().count, 1);
}

TEST(ScriptContext, SendingEventsChecksTheFields) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Fields", {
            phase = "Update",
            query = {},
            events = { send = { "test.ctx.Boom" } },
            resources = { write = { "test.ctx.Score" } },
            run = function(ctx)
                local function fails(pattern, f)
                    local ok, message = pcall(f)
                    assert(not ok and string.find(message, pattern), tostring(message))
                end
                fails("power", function() ctx.events.send("test.ctx.Boom", { power = "loud" }) end)
                fails("nope", function() ctx.events.send("test.ctx.Boom", { nope = 1 }) end)
                fails("not read", function() ctx.events.read("test.ctx.Boom") end)
                ctx.resources["test.ctx.Score"].count = 1
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.score().count, 1);
    rtype::ecs::EventCursor cursor;
    EXPECT_TRUE(fixture.app.getWorld().getEvents().read<Boom>(cursor).empty());  // a refused send sends nothing
}

TEST(ScriptContext, TheSchedulerSeesTheResourceAndEventAccessOfAScriptSystem) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Access", {
            phase = "Update",
            query = {},
            resources = { read = { "rtype.Time" }, write = { "test.ctx.Score" } },
            events = { read = { "test.ctx.Boom" }, send = { "test.ctx.Boom" } },
            run = function(ctx) end,
        })
    )"),
              "");

    const auto* system = fixture.app.getSchedules().getSchedule(rtype::ecs::Phase::kUpdate).findSystem("game.Access");
    ASSERT_NE(system, nullptr);
    const auto& registry = fixture.app.getWorld().getResources().getRegistry();
    const auto resources = system->getAccess().getResources();
    // Res<Time>, which every script system takes for ctx.delta, and the declared ones.
    const auto find = [&resources](const rtype::ecs::ResourceId id) {
        for (const auto& access : resources) {
            if (access.id == id) {
                return access.write ? 2 : 1;
            }
        }
        return 0;
    };
    EXPECT_EQ(find(registry.findByName("rtype.Time")->type.id), 1);
    EXPECT_EQ(find(registry.findByName("test.ctx.Score")->type.id), 2);
    EXPECT_EQ(find(registry.findByName("test.ctx.Empty")->type.id), 0);
    const auto events = system->getAccess().getEvents();
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events.front().id, fixture.app.getWorld().getEvents().getRegistry().findByName("test.ctx.Boom")->type.id);
    EXPECT_TRUE(events.front().write);  // read and sent: it writes
}

TEST(ScriptContext, CtxTimeGivesTheValuesOfTheTimeResource) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.ReadCtxTime", {
            phase = "Update",
            query = {},
            resources = { write = { "test.ctx.Score" } },
            run = function(ctx)
                local score = ctx.resources["test.ctx.Score"]
                score.value = ctx.time.elapsed
                score.count = ctx.time.tick * 100 + ctx.time.delta * 10 + ctx.time.fixedDelta
            end,
        })
    )"),
              "");

    fixture.app.runFrame();
    fixture.app.runFrame();

    EXPECT_FLOAT_EQ(fixture.score().value, 2.F);  // two frames of one second
    EXPECT_EQ(fixture.score().count, 2 * 100 + 10 + 1);
}

TEST(ScriptContext, CtxDeltaAndFixedDeltaStillWorkBesideCtxTime) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.CompareTime", {
            phase = "Update",
            query = {},
            resources = { write = { "test.ctx.Score" } },
            run = function(ctx)
                local same = ctx.delta == ctx.time.delta and ctx.fixedDelta == ctx.time.fixedDelta
                ctx.resources["test.ctx.Score"].count = same and 1 or 0
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.score().count, 1);
}

TEST(ScriptContext, AnUnknownCtxTimeFieldIsNil) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.UnknownTime", {
            phase = "Update",
            query = {},
            resources = { write = { "test.ctx.Score" } },
            run = function(ctx)
                ctx.resources["test.ctx.Score"].count = ctx.time.missing == nil and 1 or 0
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.score().count, 1);
}
