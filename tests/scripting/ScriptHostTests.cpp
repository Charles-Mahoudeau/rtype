/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptHostTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

#include "core/asset/AssetId.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/scripting/ScriptHost.hpp"

namespace {
using rtype::ecs::FieldType;

/// @brief A bare App and a host on it.
struct Fixture {
    rtype::ecs::App app;
    std::unique_ptr<rtype::scripting::ScriptHost> host;

    explicit Fixture(const rtype::scripting::ScriptHost::Options options = {}) {
        auto created = rtype::scripting::ScriptHost::create(app, options);
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("host-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }
};
}  // namespace

TEST(ScriptHost, ScriptDeclaresAComponent) {
    const Fixture fixture;
    EXPECT_EQ(fixture.run(R"(
        engine.ecs.component("game.Shield", {
            fields = { strength = "f32", regen = "f32" },
            defaults = { strength = 50, regen = 2 },
        })
    )"),
              "");

    const auto id = fixture.app.getWorld().getComponents().find("game.Shield");
    ASSERT_TRUE(id);
    const auto* layout = fixture.host->findComponent("game.Shield");
    ASSERT_NE(layout, nullptr);
    EXPECT_EQ(layout->id, *id);
    EXPECT_EQ(layout->size, 8U);
    ASSERT_EQ(layout->fields.size(), 2U);
    EXPECT_EQ(layout->fields[0].name, "regen");
    EXPECT_EQ(layout->fields[0].type, FieldType::kF32);
    EXPECT_EQ(layout->fields[1].name, "strength");
}

TEST(ScriptHost, DefaultsAreAppliedOverZeroedBytes) {
    const Fixture fixture;
    EXPECT_EQ(fixture.run(R"(
        engine.ecs.component("game.Stats", {
            fields = { level = "u8", ratio = "f64", alive = "bool", score = "i32" },
            defaults = { level = 7, ratio = 0.25 },
        })
    )"),
              "");

    const auto* layout = fixture.host->findComponent("game.Stats");
    ASSERT_NE(layout, nullptr);
    for (const auto& field : layout->fields) {
        const std::byte* at = layout->defaults.data() + field.offset;
        if (field.name == "level") {
            EXPECT_EQ(std::to_integer<int>(*at), 7);
        } else if (field.name == "ratio") {
            double ratio = 0;
            std::memcpy(&ratio, at, sizeof ratio);
            EXPECT_EQ(ratio, 0.25);
        } else {
            EXPECT_EQ(std::to_integer<int>(*at), 0) << field.name;
        }
    }
}

TEST(ScriptHost, VectorAndAssetIdDefaultsAreWrittenAtTheFieldOffsets) {
    const Fixture fixture;
    EXPECT_EQ(fixture.run(R"(
        engine.ecs.component("game.Sprite", {
            fields = { position = "vec2", color = "vec4", tint = "vec3", texture = "assetId" },
            defaults = {
                position = { x = 1, y = 2 },
                color = { x = 0.5, y = 0.25, z = 0, w = 1 },
                tint = vector.create(3, 4, 5),
                texture = "assets/ship.png",
            },
        })
    )"),
              "");

    const auto* layout = fixture.host->findComponent("game.Sprite");
    ASSERT_NE(layout, nullptr);
    for (const auto& field : layout->fields) {
        const std::byte* at = layout->defaults.data() + field.offset;
        if (field.name == "position") {
            std::array<float, 2> value{};
            std::memcpy(value.data(), at, sizeof value);
            EXPECT_EQ(value, (std::array<float, 2>{1.0F, 2.0F}));
        } else if (field.name == "color") {
            std::array<float, 4> value{};
            std::memcpy(value.data(), at, sizeof value);
            EXPECT_EQ(value, (std::array<float, 4>{0.5F, 0.25F, 0.0F, 1.0F}));
        } else if (field.name == "tint") {
            std::array<float, 3> value{};
            std::memcpy(value.data(), at, sizeof value);
            EXPECT_EQ(value, (std::array<float, 3>{3.0F, 4.0F, 5.0F}));
        } else {
            std::uint64_t value = 0;
            std::memcpy(&value, at, sizeof value);
            EXPECT_EQ(value, rtype::core::asset::AssetId::fromPath("assets/ship.png").value);
        }
    }
}

TEST(ScriptHost, ReplicationModeIsRead) {
    const Fixture fixture;
    EXPECT_EQ(fixture.run(R"(
        engine.ecs.component("game.Bullet", {
            fields = { owner = "entity", damage = "f32" },
            replication = "spawnOnly",
        })
    )"),
              "");

    const auto id = fixture.app.getWorld().getComponents().find("game.Bullet");
    ASSERT_TRUE(id);
    const auto* layout = fixture.host->findComponent("game.Bullet");
    ASSERT_NE(layout, nullptr);
    EXPECT_EQ(layout->size, 16U);
    EXPECT_EQ(layout->fields[0].name, "owner");
}

TEST(ScriptHost, PresenceIsReadAndDefaultsToBoth) {
    const Fixture fixture;
    EXPECT_EQ(fixture.run(R"(
        engine.ecs.component("game.Plain", { fields = { a = "f32" } })
        engine.ecs.component("game.Both", { fields = { a = "f32" }, presence = "both" })
        engine.ecs.component("game.Server", { fields = { a = "f32" }, presence = "serverOnly" })
        engine.ecs.component("game.Client", { fields = { a = "f32" }, presence = "clientOnly" })
    )"),
              "");

    const auto& registry = fixture.app.getWorld().getComponents().getRegistry();
    EXPECT_EQ(registry.findByName("game.Plain")->type.presence, rtype::ecs::Presence::kBoth);
    EXPECT_EQ(registry.findByName("game.Both")->type.presence, rtype::ecs::Presence::kBoth);
    EXPECT_EQ(registry.findByName("game.Server")->type.presence, rtype::ecs::Presence::kServerOnly);
    EXPECT_EQ(registry.findByName("game.Client")->type.presence, rtype::ecs::Presence::kClientOnly);
}

TEST(ScriptHost, UnknownPresenceIsRefused) {
    const Fixture fixture;
    const auto message = fixture.run(R"(engine.ecs.component("game.Bad", { fields = {}, presence = "nowhere" }))");
    EXPECT_NE(message.find("game.Bad"), std::string::npos) << message;
    EXPECT_NE(message.find("'presence'"), std::string::npos) << message;
    EXPECT_FALSE(fixture.app.getWorld().getComponents().find("game.Bad"));
}

TEST(ScriptHost, AClientOnlyComponentCannotBeReplicated) {
    const Fixture fixture;
    const auto message = fixture.run(R"(
        engine.ecs.component("game.Bad", { fields = { a = "f32" }, presence = "clientOnly", replication = "everyChange" })
    )");
    EXPECT_NE(message.find("game.Bad"), std::string::npos) << message;
    EXPECT_FALSE(fixture.app.getWorld().getComponents().find("game.Bad"));
}

TEST(ScriptHost, DeclaringTheSameComponentTwiceIsAccepted) {
    const Fixture fixture;
    const std::string script = R"(engine.ecs.component("game.Shield", { fields = { strength = "f32" } }))";
    EXPECT_EQ(fixture.run(script), "");
    EXPECT_EQ(fixture.run(script), "");
}

TEST(ScriptHost, AnotherLayoutUnderTheSameNameIsRefused) {
    const Fixture fixture;
    EXPECT_EQ(fixture.run(R"(engine.ecs.component("game.Shield", { fields = { strength = "f32" } }))"), "");
    EXPECT_NE(fixture.run(R"(engine.ecs.component("game.Shield", { fields = { strength = "f64" } }))"), "");
}

TEST(ScriptHost, UnknownComponentIsNotFound) {
    const Fixture fixture;
    EXPECT_EQ(fixture.host->findComponent("game.Nothing"), nullptr);
}

TEST(ScriptHost, BadDeclarationsRaiseAnErrorNamingTheComponent) {
    const Fixture fixture;
    const auto message = [&](const std::string& spec) {
        return fixture.run(R"(engine.ecs.component("game.Bad", )" + spec + ")");
    };

    EXPECT_NE(message(R"({ fields = { a = "f33" } })").find("unknown type 'f33'"), std::string::npos);
    EXPECT_NE(message(R"({ fields = { a = "vec5" } })").find("unknown type 'vec5'"), std::string::npos);
    EXPECT_NE(message(R"({ fields = { a = "vec2" }, defaults = { a = 1 } })").find("a"), std::string::npos);
    EXPECT_NE(message(R"({ fields = { a = "vec4" }, defaults = { a = { x = 1, y = 2 } } })").find("missing 'z'"),
              std::string::npos);
    EXPECT_NE(message(R"({ fields = { a = "assetId" }, defaults = { a = 3 } })").find("asset path"), std::string::npos);
    EXPECT_NE(message(R"({ fields = { a = 3 } })").find("must be a string"), std::string::npos);
    EXPECT_NE(message(R"({ })").find("needs a 'fields' table"), std::string::npos);
    EXPECT_NE(message(R"({ fields = {}, field = {} })").find("unknown option 'field'"), std::string::npos);
    EXPECT_NE(message(R"({ fields = { a = "f32" }, defaults = { b = 1 } })").find("not a field"), std::string::npos);
    EXPECT_NE(message(R"({ fields = { a = "i8" }, defaults = { a = 300 } })").find("a"), std::string::npos);
    EXPECT_NE(message(R"({ fields = { a = "f32" }, replication = "always" })").find("'replication'"),
              std::string::npos);
    EXPECT_NE(message(R"("not a table")"), "");
    EXPECT_FALSE(fixture.app.getWorld().getComponents().find("game.Bad"));
}

TEST(ScriptHost, BadDefaultNamesTheField) {
    const Fixture fixture;
    const auto message = fixture.run(R"(
        engine.ecs.component("game.Bad", { fields = { hp = "i8" }, defaults = { hp = 300 } })
    )");
    EXPECT_NE(message.find("hp"), std::string::npos);
    EXPECT_FALSE(fixture.app.getWorld().getComponents().find("game.Bad"));
}

TEST(ScriptHost, EngineFlagsAreConstants) {
    const Fixture fixture{{.isClient = true, .isServer = false, .isDebug = true}};
    EXPECT_EQ(fixture.run(R"(
        assert(engine.isClient == true)
        assert(engine.isServer == false)
        assert(engine.isDebug == true)
    )"),
              "");
}

TEST(ScriptHost, ScriptsHaveTheStandardLibraries) {
    const Fixture fixture;
    EXPECT_EQ(fixture.run(R"(assert(math.min(1, 2) == 1 and string.len("ab") == 2))"), "");
}

TEST(ScriptHost, SyntaxErrorIsReported) {
    const Fixture fixture;
    EXPECT_NE(fixture.run("engine.ecs.component("), "");
}

TEST(ScriptHost, FormsRegisteredAfterTheFirstScriptFail) {
    Fixture fixture;
    EXPECT_EQ(fixture.run("local ready = true"), "");

    fixture.app.getPluginRegistry().addScriptForms("late.Forms", "engine.late = {}");

    const std::string error = fixture.run("local again = true");
    EXPECT_NE(error.find("late.Forms"), std::string::npos) << error;
    EXPECT_NE(error.find("after the first script"), std::string::npos) << error;
}

TEST(ScriptHost, FormsRegisteredBeforeTheFirstScriptAreLoadedThenSealed) {
    Fixture fixture;
    fixture.app.getPluginRegistry().addScriptForms("early.Forms", "engine.early = { value = 1 }");

    EXPECT_EQ(fixture.run("assert(engine.early.value == 1)"), "");
    EXPECT_NE(fixture.run("engine.early.value = 2"), "");
}
