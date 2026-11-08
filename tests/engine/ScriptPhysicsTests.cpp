/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptPhysicsTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include "ScriptProbe.hpp"
#include "core/library/LibraryEntry.hpp"
#include "core/system/ExecutablePath.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/engine/Engine.hpp"
#include "rtype/engine/plugin/PluginLoader.hpp"
#include "rtype/replication/ReplicationPlugin.hpp"
#include "rtype/replication/event/ClientConnected.hpp"
#include "rtype/replication/input/InputKind.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/scripting/ScriptHost.hpp"

namespace {
constexpr int kFrames = 12;

/// @brief A server App with the physics, without gravity, and the replication plugin, one fixed tick per frame, and a
/// script host on it.
struct ServerScripts {
    rtype::engine::plugin::PluginLoader loader;  ///< The plugin libraries, which bring the setup forms on every side.
    rtype::ecs::App app{rtype::ecs::RunMode::kServer};
    std::unique_ptr<rtype::scripting::ScriptHost> host;

    ServerScripts() {
        app.addPlugin(rtype::ecs::TimePlugin::manual(std::chrono::milliseconds{20}, rtype::ecs::FixedTimestep{50, 1}));
        app.addPlugin(rtype::plugins::transform::TransformPlugin{});
        app.addPlugin(rtype::plugins::physics::PhysicsPlugin{rtype::plugins::physics::PhysicsSettings{
            .gravity = {0.0F, 0.0F},
            .maxBodies = 16,
            .maxBodyPairs = 16,
            .maxContactConstraints = 16,
            .tempAllocatorBytes = 1024 * 1024,
        }});
        app.addPlugin(rtype::replication::ReplicationPlugin{});
        loader.load(rtype::core::system::getExecutableDirectory() / rtype::engine::Engine::kPluginsDirectory);
        rtype::core::library::LibraryRegistrar registrar;
        registrar.provide(rtype::ecs::kPluginRegistryService, app.getPluginRegistry());
        loader.registerInto(registrar);
        app.getPluginRegistry().addScriptForms("test.Probe", rtype::tests::kProbeForms);
        auto created = rtype::scripting::ScriptHost::create(app, {.isServer = true});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("physics-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }

    /// @brief Runs a few frames, enough for the bodies to be created and to meet.
    void runFrames() {
        for (int frame = 0; frame < kFrames; ++frame) {
            app.runFrame();
        }
    }
};
}  // namespace

TEST(ScriptPhysics, AScriptSpawnsBodiesAndReadsTheirCollision) {
    ServerScripts server;
    ASSERT_EQ(server.run(R"(
        local spawned = false
        engine.ecs.system("game.Spawn", {
            phase = "Update",
            query = {},
            run = function(ctx)
                if spawned then return end
                spawned = true
                ctx.commands:spawn({
                    ["rtype.Transform"] = { position = { x = 0, y = 0 } },
                    ["rtype.Collider"] = { size = { x = 20, y = 20 } },
                    ["rtype.RigidBody"] = { gravityScale = 0, fixedRotation = true },
                    ["rtype.Velocity"] = { linear = { x = 600, y = 0 } },
                })
                ctx.commands:spawn({
                    ["rtype.Transform"] = { position = { x = 40, y = 0 } },
                    ["rtype.Collider"] = { size = { x = 20, y = 20 } },
                })
            end,
        })
        probe.hits = 0
        engine.ecs.system("game.Hits", {
            phase = "Update",
            query = {},
            events = { read = { "rtype.CollisionStarted" } },
            run = function(ctx)
                for collision in ctx.events.read("rtype.CollisionStarted") do
                    probe.hits += 1
                end
            end,
        })
    )"),
              "");

    server.runFrames();

    EXPECT_EQ(server.run("assert(probe.hits == 1, tostring(probe.hits))"), "");
}

TEST(ScriptPhysics, AScriptMovesAShipAndCastsARayThroughTheCalls) {
    ServerScripts server;
    ASSERT_EQ(server.run(R"(
        engine.ecs.component("game.Ship", { fields = {} })
        local spawned = false
        engine.ecs.system("game.Spawn", {
            phase = "Update",
            query = {},
            run = function(ctx)
                if spawned then return end
                spawned = true
                ctx.commands:spawn({
                    ["rtype.Transform"] = { position = { x = 100, y = 0 } },
                    ["rtype.Collider"] = { size = { x = 20, y = 200 } },
                })
                ctx.commands:spawn({
                    ["game.Ship"] = {},
                    ["rtype.Transform"] = { position = { x = 0, y = 0 } },
                    ["rtype.Collider"] = { shape = 1, radius = 10 },
                })
            end,
        })
        engine.ecs.system("game.Move", {
            phase = "FixedUpdate",
            query = { write = { "rtype.Transform" }, with = { "game.Ship" } },
            calls = { "rtype.physics.moveAndSlide", "rtype.physics.raycast" },
            after = { "rtype.Physics.SyncBodies" },
            before = { "rtype.Physics.SyncToPhysics" },
            run = function(ctx)
                for entity, transform in ctx.query do
                    transform.position = ctx.call("rtype.physics.moveAndSlide", {
                        entity = entity, from = transform.position, delta = { x = 50, y = 0 },
                    })
                    local hit = ctx.call("rtype.physics.raycast", {
                        origin = transform.position, direction = { x = 1, y = 0 }, maxDistance = 500, ignore = entity,
                    })
                    probe.shipX = transform.position.x
                    probe.wallDistance = if hit.hit then hit.distance else -1
                end
            end,
        })
    )"),
              "");

    server.runFrames();

    // Stopped a skin (0.5) away from the wall's face at x = 90, which the ray then hits.
    EXPECT_EQ(server.run("assert(math.abs(probe.shipX - 79.5) < 0.1, tostring(probe.shipX))"), "");
    EXPECT_EQ(server.run("assert(math.abs(probe.wallDistance - 10.5) < 0.1, tostring(probe.wallDistance))"), "");
}

TEST(ScriptPhysics, AServerScriptSpawnsAShipForEachClientAndReadsItsInput) {
    ServerScripts server;
    ASSERT_EQ(server.run(R"(
        engine.input.action("shoot")
        engine.input.action("move", "vector2")
        engine.ecs.system("game.Join", {
            phase = "Update",
            query = {},
            events = { read = { "net.ClientConnected" } },
            run = function(ctx)
                for client in ctx.events.read("net.ClientConnected") do
                    ctx.commands:spawn({ ["net.Owner"] = { client = client.client }, ["net.PlayerInput"] = {} })
                end
            end,
        })
        engine.ecs.system("game.Steer", {
            phase = "PostUpdate",
            query = { read = { "net.Owner" }, with = { "net.PlayerInput" } },
            calls = { "rtype.replication.readInput" },
            run = function(ctx)
                for entity, owner in ctx.query do
                    probe.seenOwner = owner.client
                    probe.seenShoot = ctx.call("rtype.replication.readInput", { entity = entity, action = "shoot" }).held
                    probe.seenMove = ctx.call("rtype.replication.readInput", { entity = entity, action = "move" }).vector
                end
            end,
        })
    )"),
              "");
    const auto& layout = server.app.getWorld().getResources().require<rtype::replication::input::InputLayout>();
    EXPECT_EQ(layout.find("shoot").kind, rtype::replication::input::InputKind::kButton);  // declared by the form
    EXPECT_EQ(layout.find("move").kind, rtype::replication::input::InputKind::kVector2);

    server.app.getWorld().getEvents().send(rtype::replication::event::ClientConnected{.client = 7});
    server.app.runFrame();

    EXPECT_EQ(server.run("assert(probe.seenOwner == 7 and probe.seenShoot == false and probe.seenMove.x == 0, "
                         "tostring(probe.seenOwner))"),
              "");
}
