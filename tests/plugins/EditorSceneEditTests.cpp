/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorSceneEditTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include "editor/scene/SceneEdit.hpp"
#include "plugins/AssetDirectory.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/CapsuleSpawn.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/entity/Name.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/resource/Resources.hpp"

namespace rtype::test::scene_edit {
struct Health {
    std::int32_t value{0};
};
RTYPE_ECS_COMPONENT(Health, "test.sceneEdit.Health", value);
}  // namespace rtype::test::scene_edit

namespace {
namespace capsule = rtype::plugins::capsule;
namespace scene = rtype::editor::scene;
using rtype::test::scene_edit::Health;

/// @brief An App with the capsule plugin over a game folder of its own, and a boss: a root with Left (and its Gun)
/// and Right.
class SceneEditTest : public ::testing::Test {
  protected:
    void SetUp() override {
        std::filesystem::create_directories(_assets);
        app.insertResource(rtype::plugins::AssetDirectory{.path = _assets});
        static_cast<void>(app.getWorld().getComponents().registerType<Health>());
        app.addPlugin(capsule::CapsulePlugin{});
        app.finishPlugins();
        capsule::Capsule boss{"actors/boss.actor"};
        set(boss, 0, 100);
        const std::uint32_t left = boss.addNode("Left", 0);
        set(boss, left, 1);
        set(boss, boss.addNode("Gun", left), 2);
        set(boss, boss.addNode("Right", 0), 3);
        capsules().define(std::move(boss));
    }

    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(_assets.parent_path(), error);
    }

    /// @brief Gives node @p node of @p capsule a Health of @p value.
    void set(capsule::Capsule& capsule, std::uint32_t node, std::int32_t value) {
        capsule.editNode(node).values.set(app.getWorld().getComponents(), Health{.value = value});
    }

    capsule::Capsules& capsules() { return *app.getWorld().getResources().getMut<capsule::Capsules>(); }
    const capsule::Capsule& boss() { return *capsules().find("actors/boss.actor"); }

    /// @return The paths of the nodes of @p capsule, in order.
    static std::vector<std::string> pathsOf(const capsule::Capsule& capsule) {
        std::vector<std::string> paths;
        for (std::size_t node = 0; node < capsule.getNodes().size(); ++node) {
            paths.push_back(capsule.getNodePath(node));
        }
        return paths;
    }

    /// @return The Health of node @p node of @p capsule.
    std::int32_t healthOf(const capsule::Capsule& capsule, std::size_t node) {
        return capsule.getNodes()[node].values.get<Health>(app.getWorld().getComponents())->value;
    }

    const std::filesystem::path _assets{std::filesystem::temp_directory_path() / "rtype-editor-scene-edit" / "assets"};
    rtype::ecs::App app;  ///< The App holding the capsules.
};
}  // namespace

TEST(EditorSceneEdit, ValidatesNodeNames) {
    EXPECT_TRUE(scene::isValidNodeName("Turret"));
    EXPECT_FALSE(scene::isValidNodeName(""));
    EXPECT_FALSE(scene::isValidNodeName("."));
    EXPECT_FALSE(scene::isValidNodeName(".."));
    EXPECT_FALSE(scene::isValidNodeName("a/b"));
}

TEST_F(SceneEditTest, ListsTheChildrenOfANode) {
    EXPECT_EQ(scene::childrenOf(boss(), 0), (std::vector<std::uint32_t>{1, 3}));
    EXPECT_EQ(scene::childrenOf(boss(), 1), (std::vector<std::uint32_t>{2}));
    EXPECT_TRUE(scene::childrenOf(boss(), 2).empty());
}

TEST_F(SceneEditTest, KnowsTheSubtreeOfANode) {
    EXPECT_TRUE(scene::isInSubtree(boss(), 2, 1));
    EXPECT_TRUE(scene::isInSubtree(boss(), 1, 1));
    EXPECT_TRUE(scene::isInSubtree(boss(), 3, 0));
    EXPECT_FALSE(scene::isInSubtree(boss(), 3, 1));
}

TEST_F(SceneEditTest, FindsAFreeNameAmongTheSiblings) {
    EXPECT_EQ(scene::uniqueChildName(boss(), 0, "Top"), "Top");
    EXPECT_EQ(scene::uniqueChildName(boss(), 0, "Left"), "Left2");
    EXPECT_EQ(scene::uniqueChildName(boss(), 1, "Left"), "Left");  // only siblings count
    EXPECT_EQ(scene::uniqueChildName(boss(), 0, "a/b"), "Node");
}

TEST_F(SceneEditTest, CopiesACapsuleWithItsValues) {
    const capsule::Capsule copy = scene::copyCapsule(boss(), app.getWorld().getComponents());
    EXPECT_EQ(copy.getPath(), "actors/boss.actor");
    EXPECT_EQ(pathsOf(copy), pathsOf(boss()));
    for (std::size_t node = 0; node < copy.getNodes().size(); ++node) {
        EXPECT_EQ(healthOf(copy, node), healthOf(boss(), node));
    }
}

TEST_F(SceneEditTest, RemovesANodeWithItsSubtree) {
    const capsule::Capsule copy = scene::copyCapsule(boss(), app.getWorld().getComponents(), {.removed = 1});
    EXPECT_EQ(pathsOf(copy), (std::vector<std::string>{".", "Right"}));
    EXPECT_EQ(healthOf(copy, 1), 3);
}

TEST_F(SceneEditTest, MovesANodeLastUnderItsNewParent) {
    const capsule::Capsule copy =
        scene::copyCapsule(boss(), app.getWorld().getComponents(), {.moved = std::pair{3U, 1U}});
    EXPECT_EQ(pathsOf(copy), (std::vector<std::string>{".", "Left", "Left/Gun", "Left/Right"}));
    EXPECT_EQ(healthOf(copy, 3), 3);
}

TEST_F(SceneEditTest, RenamesAMovedNodeWhoseNameItsNewSiblingsHave) {
    capsule::Capsule twins = scene::copyCapsule(boss(), app.getWorld().getComponents());
    static_cast<void>(scene::addChild(twins, 0, "Gun"));
    const capsule::Capsule copy =
        scene::copyCapsule(twins, app.getWorld().getComponents(), {.moved = std::pair{4U, 1U}});
    EXPECT_EQ(pathsOf(copy), (std::vector<std::string>{".", "Left", "Left/Gun", "Left/Gun2", "Right"}));
}

TEST_F(SceneEditTest, RefusesToRemoveOrMoveTheRootOrToMoveANodeUnderItself) {
    const auto& components = app.getWorld().getComponents();
    EXPECT_THROW(static_cast<void>(scene::copyCapsule(boss(), components, {.removed = 0})),
                 capsule::exceptions::CapsuleException);
    EXPECT_THROW(static_cast<void>(scene::copyCapsule(boss(), components, {.moved = std::pair{0U, 1U}})),
                 capsule::exceptions::CapsuleException);
    EXPECT_THROW(static_cast<void>(scene::copyCapsule(boss(), components, {.moved = std::pair{1U, 2U}})),
                 capsule::exceptions::CapsuleException);
}

TEST_F(SceneEditTest, AddsAChildWithAFreeNameAndAnInstance) {
    capsule::Capsule copy = scene::copyCapsule(boss(), app.getWorld().getComponents());
    const std::uint32_t added = scene::addChild(
        copy, 0, "Right",
        capsule::CapsuleRef{.id = capsule::CapsuleId::fromPath("actors/turret.actor"), .path = "actors/turret.actor"});
    EXPECT_EQ(copy.getNodePath(added), "Right2");
    ASSERT_TRUE(copy.getNodes()[added].instance.has_value());
    EXPECT_EQ(copy.getNodes()[added].instance->path, "actors/turret.actor");
}

TEST_F(SceneEditTest, RenamesANodeUnlessASiblingHasTheName) {
    capsule::Capsule copy = scene::copyCapsule(boss(), app.getWorld().getComponents());
    EXPECT_TRUE(scene::renameNode(copy, 3, "Back"));
    EXPECT_EQ(copy.getNodePath(3), "Back");
    EXPECT_FALSE(scene::renameNode(copy, 3, "Left"));
    EXPECT_FALSE(scene::renameNode(copy, 3, "a/b"));
    EXPECT_FALSE(scene::renameNode(copy, 0, "Root"));
    EXPECT_EQ(copy.getNodePath(3), "Back");
}

TEST_F(SceneEditTest, ShowsACapsuleAsTheLevelAndRespawnsItWhenCommitted) {
    ASSERT_EQ(scene::open(app.getWorld(), boss().getId()), std::nullopt);
    const rtype::ecs::Entity root = app.getWorld().getResources().get<capsule::LoadedCapsule>()->root;
    ASSERT_TRUE(app.getWorld().isAlive(root));
    EXPECT_EQ(app.getWorld().getComponents().get<rtype::ecs::Name>(root)->value.view(), "boss");

    capsule::Capsule changed = scene::copyCapsule(boss(), app.getWorld().getComponents(), {.removed = 3});
    set(changed, 0, 7);
    scene::commit(app.getWorld(), std::move(changed));
    app.runFrame();
    ASSERT_TRUE(app.getWorld().isAlive(root));
    EXPECT_EQ(app.getWorld().getComponents().get<Health>(root)->value, 7);
    EXPECT_EQ(app.getWorld().getComponents().get<rtype::ecs::Children>(root)->entities.size(), 1U);  // Left only
}

TEST_F(SceneEditTest, TellsWhyACapsuleCannotBeShown) {
    EXPECT_NE(scene::open(app.getWorld(), capsule::CapsuleId::fromPath("actors/missing.actor")), std::nullopt);
}

TEST_F(SceneEditTest, SavesACapsuleToItsFile) {
    std::filesystem::create_directories(_assets / "actors");
    ASSERT_EQ(scene::save(app.getWorld(), boss().getId(), _assets), std::nullopt);
    std::ifstream stream{_assets / "actors" / "boss.actor"};
    const std::string text{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
    EXPECT_TRUE(text.contains("[node path=\"Left/Gun\"]"));
    EXPECT_TRUE(text.contains("test.sceneEdit.Health = { value = 100 }"));
}

TEST_F(SceneEditTest, CreatesAnEmptyCapsuleFileAndReadsIt) {
    ASSERT_EQ(scene::createCapsuleFile(app.getWorld(), _assets, "scenes/level.scene"), std::nullopt);
    EXPECT_TRUE(std::filesystem::exists(_assets / "scenes" / "level.scene"));
    const capsule::Capsule* level = capsules().find("scenes/level.scene");
    ASSERT_NE(level, nullptr);
    EXPECT_EQ(level->getNodes().size(), 1U);
}

TEST_F(SceneEditTest, RefusesToCreateAFileThatIsNotACapsuleOrThatExists) {
    EXPECT_NE(scene::createCapsuleFile(app.getWorld(), _assets, "notes.txt"), std::nullopt);
    ASSERT_EQ(scene::createCapsuleFile(app.getWorld(), _assets, "a.scene"), std::nullopt);
    EXPECT_NE(scene::createCapsuleFile(app.getWorld(), _assets, "a.scene"), std::nullopt);
}
