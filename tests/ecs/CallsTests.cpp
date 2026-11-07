/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CallsTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/call/CallBuffer.hpp"
#include "rtype/ecs/call/CallInfo.hpp"
#include "rtype/ecs/call/CallTraits.hpp"
#include "rtype/ecs/call/Calls.hpp"
#include "rtype/ecs/call/DynamicCalls.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/exceptions/CallException.hpp"
#include "rtype/ecs/exceptions/MissingResourceException.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/storage/ViewAs.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/ScriptCallable.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::calls {
struct Score {
    int value{0};
};
RTYPE_ECS_RESOURCE(Score, "test.calls.Score", value);

struct Window {
    int width{0};
};
RTYPE_ECS_RESOURCE(Window, "test.calls.Window", width);

struct Marker {
    int value{0};
};
RTYPE_ECS_COMPONENT(Marker, "test.calls.Marker", value);

struct AddPoints {
    int points{1};
    float scale{1.0F};
};
RTYPE_ECS_CALL_DATA(AddPoints, "test.calls.AddPoints", points, scale);

struct Total {
    int value{0};
};
RTYPE_ECS_CALL_DATA(Total, "test.calls.Total", value);

/// @brief Adds the scaled points to the score and returns the new score.
Total addPoints(ecs::ResMut<Score> score, const AddPoints& arguments) {
    score->value += static_cast<int>(static_cast<float>(arguments.points) * arguments.scale);
    return Total{.value = score->value};
}

/// @brief Reads the score.
Total readScore(ecs::Res<Score> score) { return Total{.value = score->value}; }

/// @brief Widens the main-thread window by one.
void widen(ecs::NonSendMut<Window> window) { ++window->width; }

struct Ping {
    int value{0};
};
RTYPE_ECS_EVENT(Ping, "test.calls.Ping", value);

/// @brief Reads the pings still queued and returns how many there are.
Total countPings(ecs::EventReader<Ping> pings) { return Total{.value = static_cast<int>(pings.size())}; }

/// @brief Spawns an entity with a Marker.
void spawnMarker(ecs::Commands& commands) { commands.spawn().add(Marker{.value = 7}); }

/// @brief Call data that owns memory: a text of any length and a list.
struct Prompt {
    std::string text;
    std::vector<float> weights;
    int repeat{1};
};
RTYPE_ECS_CALL_DATA(Prompt, "test.calls.Prompt", text, weights, repeat);

/// @brief A result that owns memory.
struct Answer {
    std::string text;
    float total{0.0F};
};
RTYPE_ECS_CALL_DATA(Answer, "test.calls.Answer", text, total);

/// @brief Repeats the text and sums the weights.
Answer answer(const Prompt& prompt) {
    Answer result;
    for (int index = 0; index < prompt.repeat; ++index) {
        result.text += prompt.text;
    }
    for (const float weight : prompt.weights) {
        result.total += weight;
    }
    return result;
}

/// @brief Counts its destruction in the int it points to, if any.
struct Counted {
    int* destroyed{nullptr};
    Counted() = default;
    ~Counted() {
        if (destroyed != nullptr) {
            ++*destroyed;
        }
    }
    Counted(const Counted& other) = default;
    Counted& operator=(const Counted& other) = default;
    Counted(Counted&& other) noexcept = default;
    Counted& operator=(Counted&& other) noexcept = default;
};

/// @brief Call data that says when it is destroyed.
struct Tracked {
    std::string text;
    Counted counted;
};
RTYPE_ECS_CALL_DATA(Tracked, "test.calls.Tracked", text);
}  // namespace rtype::test::calls

namespace {
using rtype::ecs::App;
using rtype::ecs::CallId;
using rtype::ecs::CallInfo;
using rtype::ecs::CallList;
using rtype::ecs::DynamicCalls;
using rtype::ecs::FieldType;
using rtype::ecs::Phase;
using rtype::ecs::ScriptCallable;
using rtype::ecs::World;
using rtype::ecs::exceptions::CallException;
using rtype::test::calls::AddPoints;
using rtype::test::calls::Marker;
using rtype::test::calls::Score;
using rtype::test::calls::Total;
using rtype::test::calls::Window;

/// @return The bytes of a plain value.
template <typename T>
std::array<std::byte, sizeof(T)> bytesOf(const T& value) {
    std::array<std::byte, sizeof(T)> bytes{};
    std::memcpy(bytes.data(), &value, sizeof(T));
    return bytes;
}

/// @return The plain value at the start of `bytes`.
template <typename T>
T valueOf(std::span<const std::byte> bytes) {
    T value{};
    std::memcpy(&value, bytes.data(), sizeof(T));
    return value;
}

class CallsTest : public ::testing::Test {
  protected:
    void SetUp() override { app.insertResource(Score{.value = 10}); }

    /// @return The call `name`, which must be registered.
    [[nodiscard]] const CallInfo& find(const char* name) const {
        const CallInfo* info = app.getWorld().getCalls().findByName(name);
        EXPECT_NE(info, nullptr);
        return *info;
    }

    App app;  ///< The app under test.
};
}  // namespace

TEST_F(CallsTest, DescribesTheArgumentsAndTheResult) {
    const CallId id = app.addCall("test.addPoints", &rtype::test::calls::addPoints);
    const CallInfo& info = find("test.addPoints");
    EXPECT_EQ(info.id, id);
    ASSERT_TRUE(info.arguments);
    EXPECT_EQ(info.arguments->name, "test.calls.AddPoints");
    EXPECT_EQ(info.arguments->size, sizeof(AddPoints));
    ASSERT_EQ(info.arguments->fields.size(), 2U);
    EXPECT_EQ(info.arguments->fields.at(0).name, "points");
    EXPECT_EQ(info.arguments->fields.at(1).type, FieldType::kF32);
    ASSERT_TRUE(info.result);
    EXPECT_EQ(info.result->name, "test.calls.Total");
}

TEST_F(CallsTest, DescribesNothingForACallWithoutArgumentsOrResult) {
    app.addCall("test.spawnMarker", &rtype::test::calls::spawnMarker);
    const CallInfo& info = find("test.spawnMarker");
    EXPECT_FALSE(info.arguments);
    EXPECT_FALSE(info.result);
}

TEST_F(CallsTest, TakesTheAccessOfItsParameters) {
    app.addCall("test.addPoints", &rtype::test::calls::addPoints);
    app.addCall("test.readScore", &rtype::test::calls::readScore);
    app.addCall("test.widen", &rtype::test::calls::widen);
    const auto score = app.getWorld().getResources().registerType<Score>();
    ASSERT_EQ(find("test.addPoints").access.getResources().size(), 1U);
    EXPECT_EQ(find("test.addPoints").access.getResources().front().id, score);
    EXPECT_TRUE(find("test.addPoints").access.getResources().front().write);
    EXPECT_FALSE(find("test.readScore").access.getResources().front().write);
    EXPECT_FALSE(find("test.addPoints").access.isMainThread());
    EXPECT_TRUE(find("test.widen").access.isMainThread());
}

TEST_F(CallsTest, AcceptsALambdaWithoutCaptures) {
    app.addCall("test.double", [](rtype::ecs::ResMut<Score> score) { score->value *= 2; });
    app.getWorld().getCalls().invoke(app.getWorld(), find("test.double").id, {}, {});
    EXPECT_EQ(app.getWorld().getResources().require<Score>().value, 20);
}

TEST_F(CallsTest, InvokesACallWithItsArgumentsAndWritesItsResult) {
    const CallId id = app.addCall("test.addPoints", &rtype::test::calls::addPoints);
    const auto arguments = bytesOf(AddPoints{.points = 3, .scale = 2.0F});
    std::array<std::byte, sizeof(Total)> result{};
    app.getWorld().getCalls().invoke(app.getWorld(), id, arguments, result);
    EXPECT_EQ(app.getWorld().getResources().require<Score>().value, 16);
    EXPECT_EQ(valueOf<Total>(result).value, 16);
}

TEST_F(CallsTest, ReadsTheQueuedEventsAtEveryStandaloneInvocation) {
    const CallId id = app.addCall("test.countPings", &rtype::test::calls::countPings);
    app.getWorld().getEvents().send(rtype::test::calls::Ping{.value = 1});
    for (int i = 0; i < 2; ++i) {
        std::array<std::byte, sizeof(Total)> result{};
        app.getWorld().getCalls().invoke(app.getWorld(), id, {}, result);
        EXPECT_EQ(valueOf<Total>(result).value, 1);
    }
}

TEST_F(CallsTest, SharesTheBodyOfACallListedTwice) {
    const CallId id = app.addCall("test.countPings", &rtype::test::calls::countPings);
    app.getWorld().getEvents().send(rtype::test::calls::Ping{.value = 1});
    int total = 0;
    ScriptCallable callable{[&total](const DynamicCalls calls) {
        for (std::size_t i = 0; i < calls.getCount(); ++i) {
            std::array<std::byte, sizeof(Total)> result{};
            calls.invoke(i, {}, result);
            total += valueOf<Total>(result).value;
        }
    }};
    app.addSystem(Phase::kUpdate, "test.useCalls", std::move(callable), CallList{{id, id}});
    app.runFrame();
    EXPECT_EQ(total, 1);
}

TEST_F(CallsTest, AppliesTheCommandsOfACallRightAfterIt) {
    const CallId id = app.addCall("test.spawnMarker", &rtype::test::calls::spawnMarker);
    app.getWorld().getCalls().invoke(app.getWorld(), id, {}, {});
    EXPECT_EQ(app.getWorld().getEntityCount(), 1U);
}

TEST_F(CallsTest, RefusesANameTakenTwice) {
    app.addCall("test.addPoints", &rtype::test::calls::addPoints);
    EXPECT_THROW(app.addCall("test.addPoints", &rtype::test::calls::readScore), CallException);
}

TEST_F(CallsTest, RefusesAnEmptyName) { EXPECT_THROW(app.addCall("", &rtype::test::calls::readScore), CallException); }

TEST_F(CallsTest, RefusesAnUnknownIdOrBytesOfTheWrongSize) {
    const CallId id = app.addCall("test.addPoints", &rtype::test::calls::addPoints);
    std::array<std::byte, sizeof(Total)> result{};
    World& world = app.getWorld();
    EXPECT_THROW(world.getCalls().invoke(world, id + 1, {}, {}), CallException);
    EXPECT_THROW(world.getCalls().invoke(world, id, {}, result), CallException);
    const auto arguments = bytesOf(AddPoints{});
    EXPECT_THROW(world.getCalls().invoke(world, id, arguments, {}), CallException);
}

TEST_F(CallsTest, ThrowsWhenAResourceHasNoValue) {
    const CallId id = app.addCall("test.widen", &rtype::test::calls::widen);
    EXPECT_THROW(app.getWorld().getCalls().invoke(app.getWorld(), id, {}, {}),
                 rtype::ecs::exceptions::MissingResourceException);
}

TEST_F(CallsTest, FindsNothingForAnUnknownName) {
    EXPECT_EQ(app.getWorld().getCalls().findByName("test.unknown"), nullptr);
    EXPECT_EQ(app.getWorld().getCalls().findById(0), nullptr);
}

TEST_F(CallsTest, GivesASystemTheCallsItDeclared) {
    const CallId add = app.addCall("test.addPoints", &rtype::test::calls::addPoints);
    const CallId spawn = app.addCall("test.spawnMarker", &rtype::test::calls::spawnMarker);
    ScriptCallable callable{[](const DynamicCalls calls) {
        ASSERT_EQ(calls.getCount(), 2U);
        EXPECT_EQ(calls.getInfo(0).name, "test.addPoints");
        const auto arguments = bytesOf(AddPoints{.points = 5});
        std::array<std::byte, sizeof(Total)> result{};
        calls.invoke(0, arguments, result);
        EXPECT_EQ(valueOf<Total>(result).value, 15);
        calls.invoke(1, {}, {});
    }};
    app.addSystem(Phase::kUpdate, "test.useCalls", std::move(callable), CallList{{add, spawn}});
    app.runFrame();
    EXPECT_EQ(app.getWorld().getResources().require<Score>().value, 15);
    EXPECT_EQ(app.getWorld().getEntityCount(), 1U);  // the system's commands were flushed after its phase
}

TEST_F(CallsTest, AddsTheAccessOfItsCallsToTheSystem) {
    const CallId add = app.addCall("test.addPoints", &rtype::test::calls::addPoints);
    const CallId widen = app.addCall("test.widen", &rtype::test::calls::widen);
    ScriptCallable callable{[](const DynamicCalls /*calls*/) {}};
    rtype::ecs::System system{app.getWorld(), "test.useCalls", std::move(callable), CallList{{add, widen}}};
    const auto score = app.getWorld().getResources().registerType<Score>();
    const rtype::ecs::SystemAccess& access = system.getAccess();
    ASSERT_EQ(access.getResources().size(), 2U);
    EXPECT_EQ(access.getResources().front().id, score);
    EXPECT_TRUE(access.getResources().front().write);
    EXPECT_TRUE(access.isMainThread());
}

TEST_F(CallsTest, RefusesASystemWithAnUnknownCall) {
    ScriptCallable callable{[](const DynamicCalls /*calls*/) {}};
    EXPECT_THROW((rtype::ecs::System{app.getWorld(), "test.useCalls", std::move(callable), CallList{{3}}}),
                 CallException);
}

TEST(SystemAccessMergeTest, MergesEveryKindAndTheMainThreadNeed) {
    rtype::ecs::SystemAccess first;
    first.addComponent(1, false);
    first.addResource(2, false);
    rtype::ecs::SystemAccess second;
    second.addComponent(1, true);
    second.addEvent(4, true);
    second.requireMainThread();
    first.merge(second);
    ASSERT_EQ(first.getComponents().size(), 1U);
    EXPECT_TRUE(first.getComponents().front().write);
    EXPECT_EQ(first.getResources().size(), 1U);
    EXPECT_EQ(first.getEvents().size(), 1U);
    EXPECT_TRUE(first.isMainThread());
}

// ---- call data that owns memory ----

TEST_F(CallsTest, DescribesATextAndAListAsListFields) {
    app.addCall("test.answer", &rtype::test::calls::answer);
    const CallInfo& info = find("test.answer");
    ASSERT_TRUE(info.arguments.has_value());
    ASSERT_EQ(info.arguments->fields.size(), 3U);
    EXPECT_EQ(info.arguments->fields.front().type, FieldType::kText);
    EXPECT_NE(info.arguments->fields.front().list, nullptr);
    EXPECT_NE(info.arguments->hooks.destruct, nullptr);
    ASSERT_TRUE(info.result.has_value());
    EXPECT_EQ(info.result->fields.front().type, FieldType::kText);
}

TEST_F(CallsTest, PassesALongTextAndAListBothWays) {
    using rtype::test::calls::Answer;
    using rtype::test::calls::Prompt;
    const CallId id = app.addCall("test.answer", &rtype::test::calls::answer);
    const CallInfo& info = find("test.answer");
    const rtype::ecs::CallBuffer arguments{info.arguments};
    const rtype::ecs::CallBuffer result{info.result};
    auto* prompt = rtype::ecs::storage::viewAs<Prompt>(arguments.get());
    EXPECT_EQ(prompt->repeat, 1);  // built from its defaults
    prompt->text = std::string(5000, 'a');
    prompt->weights = {1.0F, 2.5F};
    prompt->repeat = 2;

    app.getWorld().getCalls().invoke(app.getWorld(), id, arguments.get(), result.get());

    const auto* answer = rtype::ecs::storage::viewAs<Answer>(std::span<const std::byte>{result.get()});
    EXPECT_EQ(answer->text.size(), 10000U);
    EXPECT_FLOAT_EQ(answer->total, 3.5F);
    EXPECT_EQ(prompt->text.size(), 5000U);  // the arguments are read, not moved from
}

TEST_F(CallsTest, ABufferDestroysTheValueItBuilt) {
    using rtype::test::calls::Tracked;
    const std::optional<rtype::ecs::TypeInfo> type{rtype::ecs::TypeTraits<Tracked>::makeTypeInfo()};
    int destroyed = 0;
    {
        const rtype::ecs::CallBuffer buffer{type};
        auto* tracked = rtype::ecs::storage::viewAs<Tracked>(buffer.get());
        tracked->counted.destroyed = &destroyed;
        tracked->text = std::string(1000, 'x');
    }
    EXPECT_EQ(destroyed, 1);
    const rtype::ecs::CallBuffer none{std::nullopt};
    EXPECT_TRUE(none.get().empty());
}

TEST_F(CallsTest, RefusesMisalignedBytesForCallDataThatOwnsMemory) {
    const CallId id = app.addCall("test.answer", &rtype::test::calls::answer);
    const CallInfo& info = find("test.answer");
    const rtype::ecs::CallBuffer result{info.result};
    alignas(std::max_align_t) std::array<std::byte, sizeof(rtype::test::calls::Prompt) + 1> raw{};
    const std::span<const std::byte> shifted{raw.data() + 1, sizeof(rtype::test::calls::Prompt)};
    EXPECT_THROW(app.getWorld().getCalls().invoke(app.getWorld(), id, shifted, result.get()), CallException);
}
