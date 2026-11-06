/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScheduleSetsTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/exceptions/ScheduleException.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/schedule/Schedule.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::schedule_sets {
/// @brief A system with no access: only its place in the order matters.
void idle() {}

/// @brief A schedule of the Render phase on its own world.
class ScheduleSetsTest : public ::testing::Test {
  protected:
    /// @return The systems in the order they run, as strings.
    [[nodiscard]] std::vector<std::string> order() {
        _schedule.build();
        std::vector<std::string> names;
        for (const std::string_view name : _schedule.getOrder()) {
            names.emplace_back(name);
        }
        return names;
    }

    /// @return Whether `first` runs before `second` in the built order.
    [[nodiscard]] bool runsBefore(std::string_view first, std::string_view second) {
        const std::vector<std::string> names = order();
        return std::ranges::find(names, first) < std::ranges::find(names, second);
    }

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
                                   rtype::ecs::Phase::kRender};  ///< Under test.
};
}  // namespace rtype::test::schedule_sets

using rtype::test::schedule_sets::idle;
using rtype::test::schedule_sets::ScheduleSetsTest;

TEST_F(ScheduleSetsTest, AMemberFollowsItsSetsConstraints) {
    // Names chosen so that, by name alone, the member would run first.
    _schedule.addSystem("z.begin", &idle);
    _schedule.addSystem("z.end", &idle);
    _schedule.configureSet("draw").after("z.begin").before("z.end");
    _schedule.addSystem("a.text", &idle).inSet("draw");
    EXPECT_EQ(order(), (std::vector<std::string>{"z.begin", "a.text", "z.end"}));
}

TEST_F(ScheduleSetsTest, OrdersASetAfterAnotherSet) {
    _schedule.configureSet("overlay").after("draw");
    _schedule.addSystem("a.debug", &idle).inSet("overlay");
    _schedule.addSystem("b.map", &idle).inSet("draw");
    _schedule.addSystem("c.text", &idle).inSet("draw");
    EXPECT_TRUE(runsBefore("b.map", "a.debug"));
    EXPECT_TRUE(runsBefore("c.text", "a.debug"));
}

TEST_F(ScheduleSetsTest, KeepsTheOrderThroughAnEmptySet) {
    _schedule.addSystem("z.first", &idle).before("stage");
    _schedule.addSystem("a.second", &idle).after("stage");
    _schedule.configureSet("stage");
    EXPECT_EQ(order(), (std::vector<std::string>{"z.first", "a.second"}));
}

TEST_F(ScheduleSetsTest, OrdersASystemAgainstASet) {
    _schedule.addSystem("a.present", &idle).after("draw");
    _schedule.addSystem("b.map", &idle).inSet("draw");
    _schedule.addSystem("c.text", &idle).inSet("draw");
    EXPECT_TRUE(runsBefore("b.map", "a.present"));
    EXPECT_TRUE(runsBefore("c.text", "a.present"));
}

TEST_F(ScheduleSetsTest, ASystemInSeveralSetsFollowsEach) {
    _schedule.addSystem("z.camera", &idle);
    _schedule.addSystem("z.present", &idle);
    _schedule.configureSet("world").after("z.camera");
    _schedule.configureSet("ui").before("z.present");
    _schedule.addSystem("a.hud", &idle).inSet("world").inSet("ui");
    EXPECT_EQ(order(), (std::vector<std::string>{"z.camera", "a.hud", "z.present"}));
}

TEST_F(ScheduleSetsTest, BuildsWithAnUnconfiguredSetAndAnEmptyOne) {
    _schedule.addSystem("a.map", &idle).inSet("joined.only");
    _schedule.configureSet("configured.only").after("a.map");
    EXPECT_EQ(buildError(), "");
    EXPECT_TRUE(_schedule.containsSet("joined.only"));
    EXPECT_TRUE(_schedule.containsSet("configured.only"));
    EXPECT_FALSE(_schedule.containsSet("a.map"));
}

TEST_F(ScheduleSetsTest, ListsOnlyTheSystemsInTheOrder) {
    _schedule.configureSet("draw");
    _schedule.addSystem("a.map", &idle).inSet("draw");
    EXPECT_EQ(order(), (std::vector<std::string>{"a.map"}));
}

TEST_F(ScheduleSetsTest, RefusesANameThatIsNeitherASystemNorASet) {
    _schedule.configureSet("draw").after("rtype.camera");
    EXPECT_EQ(buildError(),
              "Set 'draw' runs after 'rtype.camera', which is neither a system nor a set of phase Render: only systems "
              "and sets of one phase can be ordered");
}

TEST_F(ScheduleSetsTest, RefusesASetNamedLikeASystem) {
    _schedule.addSystem("draw", &idle);
    _schedule.configureSet("draw");
    EXPECT_EQ(buildError(),
              "Phase Render has a system and a set both named 'draw': systems and sets share their names");
}

TEST_F(ScheduleSetsTest, RefusesACycleThroughASetAndNamesIt) {
    _schedule.addSystem("a.map", &idle).inSet("draw").after("overlay");
    _schedule.configureSet("overlay").after("draw");
    const std::string message = buildError();
    EXPECT_NE(message.find("form a cycle"), std::string::npos) << message;
    EXPECT_NE(message.find("set 'overlay'"), std::string::npos) << message;
    EXPECT_NE(message.find("set 'draw'"), std::string::npos) << message;
}

TEST_F(ScheduleSetsTest, RebuildsWhenASetChanges) {
    _schedule.addSystem("a.map", &idle).inSet("draw");
    _schedule.build();
    EXPECT_TRUE(_schedule.isBuilt());
    _schedule.configureSet("draw").before("missing");
    EXPECT_FALSE(_schedule.isBuilt());
    EXPECT_NE(buildError(), "");
}
