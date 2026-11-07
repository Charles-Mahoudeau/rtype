/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleTextTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "plugins/GlmReflection.hpp"      // IWYU pragma: keep
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"
#include "plugins/capsule/format/CapsuleText.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/type/String.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::capsule_text {
struct Health {
    std::int32_t value{1};
};
RTYPE_ECS_COMPONENT(Health, "test.text.Health", value);

struct Pose {
    glm::vec2 position{0.F};
    float rotation{0.F};
};
RTYPE_ECS_COMPONENT(Pose, "test.text.Pose", position, rotation);

struct Sprite {
    core::asset::AssetId texture;
    std::int16_t layer{0};
    bool flip{false};
};
RTYPE_ECS_COMPONENT(Sprite, "test.text.Sprite", texture, layer, flip);

struct Label {
    std::string text;
    std::vector<std::int32_t> points;
    std::array<float, 2> pair{};
    ecs::String tag;
};
RTYPE_ECS_COMPONENT(Label, "test.text.Label", text, points, pair, tag);

struct Wide {
    double big{0.0};
    std::uint8_t small{0};
    std::int64_t negative{0};
};
RTYPE_ECS_COMPONENT(Wide, "test.text.Wide", big, small, negative);

struct Target {
    ecs::Entity entity;
};
RTYPE_ECS_COMPONENT(Target, "test.text.Target", entity);

struct Enemy {};
RTYPE_ECS_COMPONENT(Enemy, "test.text.Enemy");
}  // namespace rtype::test::capsule_text

namespace capsule = rtype::plugins::capsule;
namespace format = rtype::plugins::capsule::format;
namespace tt = rtype::test::capsule_text;

namespace {
/// @brief A world with the test components registered, and capsules to read into.
class CapsuleTextTest : public ::testing::Test {
  protected:
    void SetUp() override {
        rtype::ecs::Components& components = world.getComponents();
        static_cast<void>(components.registerType<tt::Health>());
        static_cast<void>(components.registerType<tt::Pose>());
        static_cast<void>(components.registerType<tt::Sprite>());
        static_cast<void>(components.registerType<tt::Label>());
        static_cast<void>(components.registerType<tt::Wide>());
        static_cast<void>(components.registerType<tt::Target>());
        static_cast<void>(components.registerType<tt::Enemy>());
    }

    /// @return The capsule `text` describes, at `path`.
    capsule::Capsule read(std::string_view text, std::string_view path = "actors/test.actor") {
        return format::readCapsule(path, text, world.getComponents(), capsules);
    }

    /// @return The message reading `text` fails with, or an empty string if it does not fail.
    std::string errorOf(std::string_view text) {
        try {
            static_cast<void>(read(text));
        } catch (const capsule::exceptions::CapsuleException& error) {
            return error.what();
        }
        return "";
    }

    /// @return A node's value of `T`, or nullptr.
    template <typename T>
    const T* get(const capsule::Capsule& read, std::size_t node) const {
        return read.getNodes()[node].values.get<T>(world.getComponents());
    }

    rtype::ecs::World world;     ///< Holds the components.
    capsule::Capsules capsules;  ///< Instanced capsules and asset paths.
};
}  // namespace

TEST_F(CapsuleTextTest, ReadsTheNodesAndTheirValues) {
    const capsule::Capsule boss = read(R"(
        [capsule format=1]
        # a boss with a turret
        [node path="."]
        test.text.Health = { value = 50 }
        test.text.Enemy = {}

        [node path="Turret" instance="actors/./turret.actor" side=both]
        test.text.Pose = { position = (-40, 0.5), rotation = 1.25 }

        [node path="Turret/Muzzle" side=client]
        test.text.Label = { text = "fire \"now\"", points = [1, -2, 3], pair = [0.5, 2], tag = "muzzle" }
    )");
    EXPECT_EQ(boss.getPath(), "actors/test.actor");
    const auto nodes = boss.getNodes();
    ASSERT_EQ(nodes.size(), 3U);
    EXPECT_EQ(get<tt::Health>(boss, 0)->value, 50);
    EXPECT_TRUE(nodes[0].values.has(*world.getComponents().find("test.text.Enemy")));
    EXPECT_EQ(nodes[1].name, "Turret");
    ASSERT_TRUE(nodes[1].instance);
    EXPECT_EQ(nodes[1].instance->path, "actors/turret.actor");  // normalized
    EXPECT_EQ(nodes[1].side, rtype::ecs::Side::kBoth);
    EXPECT_FLOAT_EQ(get<tt::Pose>(boss, 1)->position.x, -40.F);
    EXPECT_FLOAT_EQ(get<tt::Pose>(boss, 1)->position.y, 0.5F);
    EXPECT_FLOAT_EQ(get<tt::Pose>(boss, 1)->rotation, 1.25F);
    EXPECT_EQ(nodes[2].parent, 1U);
    EXPECT_EQ(nodes[2].side, rtype::ecs::Side::kClient);
    const tt::Label* label = get<tt::Label>(boss, 2);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->text, "fire \"now\"");
    EXPECT_EQ(label->points, (std::vector<std::int32_t>{1, -2, 3}));
    EXPECT_FLOAT_EQ(label->pair[1], 2.F);
    EXPECT_EQ(label->tag.view(), "muzzle");
}

TEST_F(CapsuleTextTest, StartsAComponentFromItsDefaults) {
    const capsule::Capsule read = this->read(R"([node path="."]
test.text.Health = {}
test.text.Wide = { big = 1e300, small = 255, negative = -9000000000 })");
    EXPECT_EQ(get<tt::Health>(read, 0)->value, 1);  // the C++ default
    EXPECT_DOUBLE_EQ(get<tt::Wide>(read, 0)->big, 1e300);
    EXPECT_EQ(get<tt::Wide>(read, 0)->small, 255);
    EXPECT_EQ(get<tt::Wide>(read, 0)->negative, -9000000000);
}

TEST_F(CapsuleTextTest, MergesAnInstanceNodesFieldsOntoTheInstancedRoot) {
    capsules.define(read(R"([node path="."]
test.text.Pose = { position = (1, 2), rotation = 0.5 }
test.text.Health = { value = 5 })",
                         "actors/turret.actor"));
    std::vector<std::string> loaded;
    const capsule::Capsule boss =
        format::readCapsule("actors/boss.actor", R"([node path="."]
[node path="Turret" instance="actors/turret.actor"]
test.text.Pose = { rotation = 3 }
test.text.Wide = { small = 2 })",
                            world.getComponents(), capsules, [&](std::string_view path) { loaded.emplace_back(path); });
    EXPECT_EQ(loaded, (std::vector<std::string>{"actors/turret.actor"}));  // asked before it is looked up
    const tt::Pose* pose = get<tt::Pose>(boss, 1);
    EXPECT_FLOAT_EQ(pose->position.x, 1.F);  // the turret's
    EXPECT_FLOAT_EQ(pose->rotation, 3.F);    // the node's
    EXPECT_EQ(get<tt::Wide>(boss, 1)->small, 2);
    EXPECT_FALSE(boss.getNodes()[1].values.has(*world.getComponents().find("test.text.Health")));  // only what it sets
}

TEST_F(CapsuleTextTest, ReadsAssetPathsAndRemembersThem) {
    const capsule::Capsule read = this->read(R"([node path="."]
test.text.Sprite = { texture = "sprites/./ship.png", layer = -3, flip = true }
[node path="Raw"]
test.text.Sprite = { texture = 0x2a })");
    const tt::Sprite* sprite = get<tt::Sprite>(read, 0);
    EXPECT_EQ(sprite->texture, rtype::core::asset::AssetId::fromPath("sprites/ship.png"));
    EXPECT_EQ(sprite->layer, -3);
    EXPECT_TRUE(sprite->flip);
    ASSERT_NE(capsules.findAssetPath(sprite->texture), nullptr);
    EXPECT_EQ(*capsules.findAssetPath(sprite->texture), "sprites/ship.png");
    EXPECT_EQ(get<tt::Sprite>(read, 1)->texture.value, 42U);
}

TEST_F(CapsuleTextTest, ReadsAnEmptyFileAsARootOnly) {
    const capsule::Capsule read = this->read("# nothing yet\n");
    EXPECT_EQ(read.getNodes().size(), 1U);
}

TEST_F(CapsuleTextTest, WritesTheFieldsThatDifferFromWhereReadingStarts) {
    capsules.define(read(R"([node path="."]
test.text.Pose = { position = (1, 2) })",
                         "actors/turret.actor"));
    const capsule::Capsule boss = read(R"([node path="."]
test.text.Health = { value = 50 }
test.text.Enemy = {}
test.text.Sprite = { texture = "sprites/boss.png" }
[node path="Left" instance="actors/turret.actor" side=client]
test.text.Pose = { rotation = 0.1 }
[node path="Left/Tag"]
test.text.Label = { text = "a\nb", points = [4], tag = "x" })");
    EXPECT_EQ(format::writeCapsule(boss, world.getComponents(), capsules), R"([capsule format=1]

[node path="."]
test.text.Health = { value = 50 }
test.text.Enemy = {}
test.text.Sprite = { texture = "sprites/boss.png" }

[node path="Left" instance="actors/turret.actor" side=client]
test.text.Pose = { rotation = 0.1 }

[node path="Left/Tag"]
test.text.Label = { text = "a\nb", points = [4], tag = "x" }
)");
}

TEST_F(CapsuleTextTest, ReadsWhatItWritesBackTheSame) {
    const capsule::Capsule first = read(R"([node path="."]
test.text.Pose = { position = (-0.3, 1e-7), rotation = 3.14159 }
test.text.Wide = { big = 0.1, small = 7, negative = -1 }
test.text.Sprite = { texture = 0x123456789abcdef0, layer = 2 }
[node path="A" side=both]
test.text.Label = { text = "tab\there", pair = [1.5, -2.25] }
[node path="A/B"]
test.text.Health = { value = -7 })");
    const std::string written = format::writeCapsule(first, world.getComponents(), capsules);
    const capsule::Capsule second = read(written);
    EXPECT_EQ(format::writeCapsule(second, world.getComponents(), capsules), written);
    EXPECT_EQ(get<tt::Pose>(second, 0)->position.y, 1e-7F);  // floats read back exactly
    EXPECT_EQ(get<tt::Wide>(second, 0)->big, 0.1);
    EXPECT_EQ(get<tt::Sprite>(second, 0)->texture.value, 0x123456789abcdef0U);
}

TEST_F(CapsuleTextTest, RefusesWhatItCannotRead) {
    const std::vector<std::pair<std::string_view, std::string_view>> cases{
        {R"([node path="."]
nope.Missing = {})",
         "actors/test.actor:2:1: unknown component 'nope.Missing'"},
        {R"([node path="."]
test.text.Health = { hp = 1 })",
         "actors/test.actor:2:22: component 'test.text.Health' has no field 'hp'"},
        {R"([node path="."]
test.text.Health = { value = "a" })",
         "actors/test.actor:2:30: field 'value' of 'test.text.Health' takes a number, not a string"},
        {R"([node path="."]
test.text.Wide = { small = 256 })",
         "actors/test.actor:2:28: field 'small' of 'test.text.Wide' takes a whole number from 0 to 255, not 256"},
        {R"([node path="."]
test.text.Pose = { position = (1) })",
         "actors/test.actor:2:31: field 'position' of 'test.text.Pose' takes a vector of 2, not 1"},
        {R"([node path="Boss"])", R"(actors/test.actor:1:12: the first node is the root: path=".")"},
        {"[node path=\".\"]\n[node path=\"A/B\"]", "actors/test.actor:2:12: 'A/B' has no parent node 'A' above it"},
        {"[node path=\".\"]\n[node path=\"A\"]\n[node path=\"A\"]",
         "actors/test.actor:3:12: Capsule 'actors/test.actor': '.' already has a child named 'A'"},
        {"[node path=\".\"]\n[node path=\"A\" side=up]",
         "actors/test.actor:2:21: unknown side 'up': expected server, client or both"},
        {"[node path=\".\" side=client]",
         "actors/test.actor:1:21: the root is spawned on every side: it takes no 'side'"},
        {"[node path=\".\"]\ntest.text.Target = { entity = 1 }",
         "actors/test.actor:2:31: field 'entity' of 'test.text.Target' cannot be set in a capsule file yet"},
        {"[capsule format=2]", "actors/test.actor:1:17: format 2 is not known: this engine reads format 1"},
        {"[node path=\".\"]\ntest.text.Health = {}\ntest.text.Health = {}",
         "actors/test.actor:3:1: component 'test.text.Health' is set twice on this node"},
        {"[node path=\".\"]\ntest.text.Label = { text = \"open }",
         "actors/test.actor:2:28: this string has no closing quote"},
        {"[node path=\".\"] @", "actors/test.actor:1:17: unexpected character '@'"},
        {"[scene]", "actors/test.actor:1:2: unknown section 'scene': expected 'capsule' or 'node'"},
        {"[node]",
         R"(actors/test.actor:1:1: a [node] needs a path: path="." for the root, path="Boss/Turret" below it)"},
        {"[node path=\".\"]\ntest.text.Health = { value = 1",
         "actors/test.actor:2:31: expected ',' or '}' to end the fields of 'test.text.Health', found the end of the "
         "file"},
    };
    for (const auto& [text, message] : cases) {
        EXPECT_EQ(errorOf(text), message) << text;
    }
}

TEST(CapsuleText, KnowsTheExtensionsOfCapsuleFiles) {
    EXPECT_TRUE(format::isCapsuleFile("scenes/level1.scene"));
    EXPECT_TRUE(format::isCapsuleFile("actors/bydo.actor"));
    EXPECT_TRUE(format::isCapsuleFile("misc/thing.capsule"));
    EXPECT_FALSE(format::isCapsuleFile("sprites/ship.png"));
    EXPECT_FALSE(format::isCapsuleFile("actors/bydo.actor.bak"));
}

TEST_F(CapsuleTextTest, WritesFloatsInTheirShortestExactForm) {
    const std::vector<std::pair<std::string_view, std::string_view>> cases{
        {"-40", "-40"}, {"0.5", "0.5"}, {"0.1", "0.1"}, {"1e-7", "1e-07"}, {"1e20", "1e+20"}, {"-0", "0"},
    };
    for (const auto& [given, written] : cases) {
        const capsule::Capsule read =
            this->read(std::format("[node path=\".\"]\ntest.text.Pose = {{ rotation = {} }}\n", given));
        const std::string text = format::writeCapsule(read, world.getComponents(), capsules);
        const std::string expected = std::format("rotation = {} }}", written);
        EXPECT_TRUE(text.contains(expected) || written == "0") << text;
    }
}
