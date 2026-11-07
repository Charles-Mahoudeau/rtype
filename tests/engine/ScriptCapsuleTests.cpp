/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptCapsuleTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryState.hpp"
#include "rtype/scripting/ScriptHost.hpp"

namespace rtype::test::script_capsule {
struct Health {
    std::int32_t value;
};
RTYPE_ECS_COMPONENT(Health, "test.scriptcapsule.Health", value);
}  // namespace rtype::test::script_capsule

namespace capsule = rtype::plugins::capsule;
namespace sc = rtype::test::script_capsule;

namespace {
/// @brief A standalone App with the capsule plugin, "actors/bydo.actor" (Health 3, a "Gun" child), "scenes/one.scene"
/// and "scenes/two.scene" (a bydo each), and a script host on it.
struct ScriptApp {
    rtype::ecs::App app;
    std::unique_ptr<rtype::scripting::ScriptHost> host;

    ScriptApp() {
        app.addPlugin(capsule::CapsulePlugin{});
        static_cast<void>(app.getWorld().getComponents().registerType<sc::Health>());
        auto& capsules = *app.getWorld().getResources().getMut<capsule::Capsules>();
        capsule::Capsule bydo{"actors/bydo.actor"};
        bydo.editNode(0).values.set(app.getWorld().getComponents(), sc::Health{.value = 3});
        static_cast<void>(bydo.addNode("Gun", 0));
        capsules.define(std::move(bydo));
        for (const char* path : {"scenes/one.scene", "scenes/two.scene"}) {
            capsule::Capsule level{path};
            level.editNode(level.addNode("Bydo", 0)).instance = capsule::CapsuleRef{
                .id = capsule::CapsuleId::fromPath("actors/bydo.actor"), .path = "actors/bydo.actor"};
            capsules.define(std::move(level));
        }
        auto created = rtype::scripting::ScriptHost::create(app, {});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("capsule-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }
};
}  // namespace

TEST(ScriptCapsule, ASystemSpawnsLoadsAndReloadsCapsules) {
    ScriptApp script;
    ASSERT_EQ(script.run(R"(
        frame = 0
        engine.ecs.system("game.Levels", {
            phase = "Update",
            query = {},
            calls = { "rtype.capsule.spawn", "rtype.capsule.load", "rtype.capsule.reload" },
            run = function(ctx)
                frame += 1
                if frame == 1 then
                    bydo = ctx.call("rtype.capsule.spawn", { capsule = "actors/bydo.actor" })
                    ctx.commands:add(bydo, "test.scriptcapsule.Health", { value = 10 })   -- over the capsule's
                    first = ctx.call("rtype.capsule.load", { capsule = "scenes/one.scene" })
                elseif frame == 2 then
                    second = ctx.call("rtype.capsule.load", { capsule = "scenes/two.scene" })
                elseif frame == 3 then
                    reloaded = ctx.call("rtype.capsule.reload", { root = bydo })
                end
            end,
        })
    )"),
              "");
    rtype::ecs::World& world = script.app.getWorld();
    script.app.runFrame();
    EXPECT_EQ(world.getEntityCount(), 5U);  // the bydo and its gun, the first level, its bydo and gun
    script.app.runFrame();
    EXPECT_EQ(world.getEntityCount(), 5U);  // the first level replaced by the second
    script.app.runFrame();
    EXPECT_EQ(script.run("assert(reloaded == true)"), "");

    const auto* loaded = world.getResources().get<capsule::LoadedCapsule>();
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->capsule, rtype::core::asset::AssetId::fromPath("scenes/two.scene"));
    EXPECT_EQ(script.run("assert(first ~= second)"), "");
}

TEST(ScriptCapsule, AnOverrideAddedAfterTheSpawnReplacesTheCapsulesValue) {
    ScriptApp script;
    ASSERT_EQ(script.run(R"(
        engine.ecs.system("game.Spawn", {
            phase = "Update",
            query = {},
            calls = { "rtype.capsule.spawn" },
            run = function(ctx)
                if bydo then return end
                bydo = ctx.call("rtype.capsule.spawn", { capsule = "actors/bydo.actor" })
                ctx.commands:add(bydo, "test.scriptcapsule.Health", { value = 10 })
            end,
        })
    )"),
              "");
    script.app.runFrame();
    rtype::ecs::World& world = script.app.getWorld();
    std::vector<std::int32_t> health;
    const rtype::ecs::QueryState<const sc::Health> state{world.getComponents()};
    rtype::ecs::Query<const sc::Health> query{state, 0};
    for (auto [entity, value] : query) {
        health.push_back(value.value);
    }
    EXPECT_EQ(health, (std::vector<std::int32_t>{10}));  // the bydo's root, its Health replaced by the script
    EXPECT_EQ(world.getEntityCount(), 2U);
}

TEST(ScriptCapsule, ACallOnAnUnknownCapsuleFailsNamingIt) {
    ScriptApp script;
    const std::string error = script.run(R"(engine.call("rtype.capsule.spawn", { capsule = "actors/nothing.actor" }))");
    EXPECT_TRUE(error.contains("rtype.capsule.spawn")) << error;
    EXPECT_TRUE(error.contains("No capsule has the id")) << error;
}
