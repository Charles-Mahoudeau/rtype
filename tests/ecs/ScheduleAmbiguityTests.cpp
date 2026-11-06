/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScheduleAmbiguityTests
*/

#include <gtest/gtest.h>

#include <string>

#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/exceptions/ScheduleException.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/schedule/Schedule.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::schedule_ambiguity {
/// @brief A queue sorted before it is read: the order of its writes does not matter.
struct Queue {
    int pushed;
};
RTYPE_ECS_RESOURCE(Queue, "test.ambiguity.Queue", pushed);

/// @brief A resource whose writes do not commute.
struct Score {
    int value;
};
RTYPE_ECS_RESOURCE(Score, "test.ambiguity.Score", value);

struct Position {
    float x;
};
RTYPE_ECS_COMPONENT(Position, "test.ambiguity.Position", x);

void pushMap(rtype::ecs::ResMut<Queue> queue) { ++queue->pushed; }
void pushText(rtype::ecs::ResMut<Queue> queue) { ++queue->pushed; }
void pushAndScore(rtype::ecs::ResMut<Queue> queue, rtype::ecs::ResMut<Score> score) {
    ++queue->pushed;
    ++score->value;
}
void scoreToo(rtype::ecs::ResMut<Score> score) { ++score->value; }
void pushAndMove(rtype::ecs::ResMut<Queue> queue, rtype::ecs::Query<Position> positions) {
    ++queue->pushed;
    static_cast<void>(positions);
}
void moveToo(rtype::ecs::Query<Position> positions) { static_cast<void>(positions); }

/// @brief A schedule of the Render phase on its own world, the queue's id known.
class ScheduleAmbiguityTest : public ::testing::Test {
  protected:
    /// @return The message build() throws, or an empty string if it builds.
    [[nodiscard]] std::string buildError() {
        try {
            _schedule.build();
        } catch (const rtype::ecs::exceptions::ScheduleException& error) {
            return error.what();
        }
        return "";
    }

    rtype::ecs::World _world;  ///< The systems' world.
    rtype::ecs::Schedule _schedule{_world, rtype::ecs::RunMode::kStandalone,
                                   rtype::ecs::Phase::kRender};                  ///< Under test.
    rtype::ecs::ResourceId _queue{_world.getResources().registerType<Queue>()};  ///< The queue's id.
};
}  // namespace rtype::test::schedule_ambiguity

using rtype::test::schedule_ambiguity::ScheduleAmbiguityTest;
namespace amb = rtype::test::schedule_ambiguity;

TEST_F(ScheduleAmbiguityTest, RefusesUnorderedWritersOfAResource) {
    _schedule.addSystem("a.map", &amb::pushMap);
    _schedule.addSystem("b.text", &amb::pushText);
    const std::string message = buildError();
    EXPECT_NE(message.find("'a.map' and 'b.text' both use resource 'test.ambiguity.Queue'"), std::string::npos)
        << message;
}

TEST_F(ScheduleAmbiguityTest, AcceptsUnorderedWritersOfAnAmbiguousResource) {
    EXPECT_FALSE(_schedule.isAmbiguousResource(_queue));
    _schedule.allowAmbiguousResource(_queue);
    EXPECT_TRUE(_schedule.isAmbiguousResource(_queue));
    _schedule.addSystem("a.map", &amb::pushMap);
    _schedule.addSystem("b.text", &amb::pushText);
    EXPECT_EQ(buildError(), "");
}

TEST_F(ScheduleAmbiguityTest, StillRefusesAnotherSharedResource) {
    _schedule.allowAmbiguousResource(_queue);
    _schedule.addSystem("a.map", &amb::pushAndScore);
    _schedule.addSystem("b.score", &amb::scoreToo);
    _schedule.addSystem("c.text", &amb::pushText);
    const std::string message = buildError();
    EXPECT_NE(message.find("'a.map' and 'b.score' both use resource 'test.ambiguity.Score'"), std::string::npos)
        << message;
    EXPECT_EQ(message.find("Queue"), std::string::npos) << message;
}

TEST_F(ScheduleAmbiguityTest, StillRefusesASharedComponent) {
    _schedule.allowAmbiguousResource(_queue);
    _schedule.addSystem("a.map", &amb::pushAndMove);
    _schedule.addSystem("b.move", &amb::moveToo);
    const std::string message = buildError();
    EXPECT_NE(message.find("component 'test.ambiguity.Position'"), std::string::npos) << message;
}

TEST_F(ScheduleAmbiguityTest, AllowingTwiceIsHarmlessAndRebuilds) {
    _schedule.addSystem("a.map", &amb::pushMap);
    _schedule.build();
    _schedule.allowAmbiguousResource(_queue);
    _schedule.allowAmbiguousResource(_queue);
    EXPECT_FALSE(_schedule.isBuilt());
    _schedule.addSystem("b.text", &amb::pushText);
    EXPECT_EQ(buildError(), "");
}
