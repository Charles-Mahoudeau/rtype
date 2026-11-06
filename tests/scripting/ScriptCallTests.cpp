/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptCallTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/call/CallTraits.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/ecs/system/SystemParam.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/scripting/ScriptHost.hpp"

namespace rtype::test::script_call {
/// @brief The resource the calls below change.
struct Score {
    int value{0};
};
RTYPE_ECS_RESOURCE(Score, "test.call.Score", value);

/// @brief A main-thread resource.
struct Window {
    int width{0};
};
RTYPE_ECS_RESOURCE(Window, "test.call.Window", width);

/// @brief The arguments of `test.addPoints`.
struct AddPoints {
    int points{1};
    float scale{1.F};
};
RTYPE_ECS_CALL_DATA(AddPoints, "test.call.AddPoints", points, scale);

/// @brief A result of one field.
struct Total {
    int value{0};
};
RTYPE_ECS_CALL_DATA(Total, "test.call.Total", value);

/// @brief A result of two fields.
struct Bounds {
    int low{0};
    int high{0};
};
RTYPE_ECS_CALL_DATA(Bounds, "test.call.Bounds", low, high);

/// @brief Adds the scaled points to the score and returns the new score.
Total addPoints(ecs::ResMut<Score> score, const AddPoints& arguments) {
    score->value += static_cast<int>(static_cast<float>(arguments.points) * arguments.scale);
    return Total{.value = score->value};
}

/// @brief Reads the score.
Total readScore(ecs::Res<Score> score) { return Total{.value = score->value}; }

/// @brief The score, one below and one above.
Bounds bounds(ecs::Res<Score> score) { return Bounds{.low = score->value - 1, .high = score->value + 1}; }

/// @brief Sets the score back to zero.
void reset(ecs::ResMut<Score> score) { score->value = 0; }

/// @brief Widens the main-thread window by one.
void widen(ecs::NonSendMut<Window> window) { ++window->width; }

/// @brief Always fails.
void broken(ecs::Res<Score> /*score*/) { throw std::runtime_error{"the call broke"}; }
}  // namespace rtype::test::script_call

namespace {
using rtype::test::script_call::Score;
using rtype::test::script_call::Window;

/// @brief An App with a manual clock, the calls above and a host on it.
struct Fixture {
    rtype::ecs::App app;
    std::unique_ptr<rtype::scripting::ScriptHost> host;

    explicit Fixture(const rtype::ecs::RunMode mode = rtype::ecs::RunMode::kStandalone) : app{mode} {
        app.addPlugin(rtype::ecs::TimePlugin::manual(std::chrono::seconds{1}, rtype::ecs::FixedTimestep{1, 5}));
        app.insertResource(Score{.value = 10});
        app.getWorld().getResources().insertNonSend(Window{});
        app.addCall("test.addPoints", &rtype::test::script_call::addPoints);
        app.addCall("test.readScore", &rtype::test::script_call::readScore);
        app.addCall("test.bounds", &rtype::test::script_call::bounds);
        app.addCall("test.reset", &rtype::test::script_call::reset);
        app.addCall("test.widen", &rtype::test::script_call::widen);
        app.addCall("test.broken", &rtype::test::script_call::broken);
        auto created = rtype::scripting::ScriptHost::create(app, {});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("call-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }

    [[nodiscard]] int score() const { return app.getWorld().getResources().require<Score>().value; }
};

/// @return Whether `text` contains `part`.
[[nodiscard]] bool contains(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}
}  // namespace

TEST(ScriptCall, EngineCallGivesTheArgumentsAndReturnsTheResultField) {
    Fixture fixture;
    EXPECT_EQ(fixture.run(R"(
        local total = engine.call("test.addPoints", { points = 3, scale = 2 })
        assert(total == 16, tostring(total))
    )"),
              "");
    EXPECT_EQ(fixture.score(), 16);
}

TEST(ScriptCall, MissingArgumentFieldsKeepTheirDefaults) {
    Fixture fixture;
    EXPECT_EQ(fixture.run(R"(
        assert(engine.call("test.addPoints", { points = 4 }) == 14)
        assert(engine.call("test.addPoints") == 15)
        assert(engine.call("test.addPoints", nil) == 16)
    )"),
              "");
}

TEST(ScriptCall, EngineCallWithoutArgumentsReturnsItsResult) {
    Fixture fixture;
    EXPECT_EQ(fixture.run(R"(assert(engine.call("test.readScore") == 10))"), "");
}

TEST(ScriptCall, AResultOfSeveralFieldsIsATable) {
    Fixture fixture;
    EXPECT_EQ(fixture.run(R"(
        local bounds = engine.call("test.bounds")
        assert(bounds.low == 9 and bounds.high == 11)
    )"),
              "");
}

TEST(ScriptCall, ACallWithoutResultReturnsNothing) {
    Fixture fixture;
    EXPECT_EQ(fixture.run(R"(assert(select("#", engine.call("test.reset")) == 0))"), "");
    EXPECT_EQ(fixture.score(), 0);
}

TEST(ScriptCall, AnUnknownCallRaisesAnErrorNamingIt) {
    Fixture fixture;
    const auto error = fixture.run(R"(engine.call("test.nope"))");
    EXPECT_TRUE(contains(error, "unknown engine call 'test.nope'")) << error;
    EXPECT_TRUE(contains(fixture.run(R"(engine.call(3))"), "expects the name of an engine call"));
}

TEST(ScriptCall, BadArgumentsRaiseAnErrorNamingTheCall) {
    Fixture fixture;
    EXPECT_EQ(fixture.run(R"(
        local function failsWith(part, ...)
            local ok, message = pcall(engine.call, ...)
            assert(not ok, "the call should fail")
            assert(string.find(message, "engine call '", 1, true), message)
            assert(string.find(message, part, 1, true), message)
        end
        failsWith("expects a table of arguments", "test.addPoints", 3)
        failsWith("takes no arguments", "test.readScore", { value = 1 })
        failsWith("points", "test.addPoints", { points = "many" })
        failsWith("nothing", "test.addPoints", { nothing = 1 })
    )"),
              "");
    EXPECT_EQ(fixture.score(), 10);  // no refused call ran
}

TEST(ScriptCall, ACallThatThrowsRaisesALuauErrorNamingTheCall) {
    Fixture fixture;
    const auto error = fixture.run(R"(engine.call("test.broken"))");
    EXPECT_TRUE(contains(error, "engine call 'test.broken': the call broke")) << error;
}

TEST(ScriptCall, ASystemMakesTheCallsItDeclares) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Scorer", {
            phase = "Update",
            query = {},
            calls = { "test.addPoints" },
            run = function(ctx)
                ctx.call("test.addPoints", { points = 5 })
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.score(), 15);
}

TEST(ScriptCall, ACallTheSystemDidNotDeclareRaisesAnError) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Sneaky", {
            phase = "Update",
            query = {},
            calls = { "test.addPoints" },
            run = function(ctx)
                local ok, message = pcall(ctx.call, "test.reset")
                assert(not ok, "an undeclared call should fail")
                assert(string.find(message, "engine call 'test.reset' is not declared by system 'game.Sneaky'", 1,
                    true), message)
                ctx.call("test.addPoints")
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.score(), 11);  // the asserts passed, then the declared call ran
}

TEST(ScriptCall, AnUnknownDeclaredCallIsRefusedWhenTheScriptLoads) {
    Fixture fixture;
    const auto error = fixture.run(R"(
        engine.ecs.system("game.Lost", { phase = "Update", query = {}, calls = { "test.nope" }, run = function() end })
    )");
    EXPECT_TRUE(contains(error, "System 'game.Lost': unknown engine call 'test.nope'")) << error;
    EXPECT_TRUE(contains(fixture.run(R"(
        engine.ecs.system("game.Bad", { phase = "Update", query = {}, calls = "test.reset", run = function() end })
    )"),
                         "'calls'"));
}

TEST(ScriptCall, AnUnknownDeclaredCallIsDroppedWhenTheSystemDoesNotRunInThisApp) {
    Fixture fixture{rtype::ecs::RunMode::kServer};
    EXPECT_EQ(fixture.run(R"(
        engine.ecs.system("game.ClientOnly", {
            phase = "Update",
            query = {},
            side = "client",
            calls = { "test.playSound" },
            run = function(ctx) ctx.call("test.playSound") end,
        })
    )"),
              "");
}

TEST(ScriptCall, TheSchedulerSeesTheAccessOfTheDeclaredCalls) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Widener", {
            phase = "Update",
            query = {},
            calls = { "test.widen", "test.addPoints", "test.widen" },
            run = function(ctx) ctx.call("test.widen") end,
        })
    )"),
              "");

    const auto* system = fixture.app.getSchedules().getSchedule(rtype::ecs::Phase::kUpdate).findSystem("game.Widener");
    ASSERT_NE(system, nullptr);
    EXPECT_TRUE(system->getAccess().isMainThread());  // test.widen takes a NonSendMut
    const auto& registry = fixture.app.getWorld().getResources().getRegistry();
    const auto resources = system->getAccess().getResources();
    const auto writes = [&resources](const rtype::ecs::ResourceId id) {
        for (const auto& access : resources) {
            if (access.id == id) {
                return access.write;
            }
        }
        return false;
    };
    EXPECT_TRUE(writes(registry.findByName("test.call.Score")->type.id));
    EXPECT_TRUE(writes(registry.findByName("test.call.Window")->type.id));

    fixture.app.runFrame();

    EXPECT_EQ(fixture.app.getWorld().getResources().require<Window>().width, 1);
}
