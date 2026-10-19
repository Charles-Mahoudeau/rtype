/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ManifestTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/world/World.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/replication/MessageType.hpp"
#include "rtype/replication/ReplicationSettings.hpp"
#include "rtype/replication/exceptions/ReplicationExceptions.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/manifest/Manifest.hpp"
#include "rtype/replication/manifest/ManifestMessages.hpp"
#include "rtype/replication/manifest/ManifestReceiver.hpp"

namespace {
using rtype::ecs::ComponentId;
using rtype::ecs::ComponentInfo;
using rtype::ecs::FieldType;
using rtype::ecs::Presence;
using rtype::ecs::ReplicationMode;
using rtype::ecs::World;
using rtype::network::Bytes;
using rtype::replication::MessageType;
using rtype::replication::ReplicationSettings;
using rtype::replication::exceptions::InputLayoutException;
using rtype::replication::exceptions::ManifestException;
using rtype::replication::input::InputKind;
using rtype::replication::input::InputLayout;
using rtype::replication::manifest::Manifest;
using rtype::replication::manifest::ManifestCodec;
using rtype::replication::manifest::ManifestComponent;
using rtype::replication::manifest::ManifestEnd;
using rtype::replication::manifest::ManifestInputAction;
using rtype::replication::manifest::ManifestReceiver;

/// @brief Registers a replicated component with one `f32` field per name of `fields`, in the given world.
ComponentId addComponent(World& world, std::string name, const std::vector<std::string>& fields = {"value"},
                         const ReplicationMode replication = ReplicationMode::kEveryChange) {
    ComponentInfo info;
    info.type.name = std::move(name);
    info.type.size = fields.size() * sizeof(float);
    info.type.alignment = alignof(float);
    info.type.presence = Presence::kBoth;
    info.type.replication = replication;
    for (std::size_t index = 0; index < fields.size(); ++index) {
        info.type.fields.push_back({.name = fields[index],
                                    .type = FieldType::kF32,
                                    .offset = static_cast<std::uint32_t>(index * sizeof(float)),
                                    .count = 1});
    }
    return world.getComponents().registerType(std::move(info)).value();
}

/// @brief Sends every message of `messages` to `receiver`; true when all are accepted.
bool feed(ManifestReceiver& receiver, const std::vector<Bytes>& messages) {
    for (const Bytes& message : messages) {
        if (!receiver.receive(message)) {
            return false;
        }
    }
    return true;
}

/// @brief Two worlds registered in different orders, with the same components and a layout of two actions.
struct TwoWorlds {
    World server;
    World client;
    InputLayout serverInputs;
    InputLayout clientInputs;

    TwoWorlds() {
        addComponent(server, "game.Health");
        addComponent(server, "game.Position", {"x", "y"});
        addComponent(server, "game.Ammo");
        addComponent(client, "game.Ammo");
        addComponent(client, "game.Position", {"x", "y"});
        addComponent(client, "game.Health");
        serverInputs.add("move", InputKind::kVector2);
        serverInputs.add("fire", InputKind::kButton);
        clientInputs.add("fire", InputKind::kButton);
        clientInputs.add("move", InputKind::kVector2);
    }
};

/// @brief Builds the server's manifest and its welcome messages.
std::vector<Bytes> welcome(const TwoWorlds& setup, const ReplicationSettings& settings = {}) {
    return Manifest::build(setup.server, setup.serverInputs).encode(settings);
}
}  // namespace

TEST(Manifest, SortsComponentsByNameAndGivesShortIds) {
    TwoWorlds setup;
    const Manifest manifest = Manifest::build(setup.server, setup.serverInputs);

    ASSERT_EQ(manifest.getComponentEntries().size(), 3U);
    EXPECT_EQ(manifest.getComponentEntries()[0].name, "game.Ammo");
    EXPECT_EQ(manifest.getComponentEntries()[1].name, "game.Health");
    EXPECT_EQ(manifest.getComponentEntries()[2].name, "game.Position");
    ASSERT_EQ(manifest.getActions().size(), 2U);
    EXPECT_EQ(manifest.getActions()[0].name, "fire");
    EXPECT_EQ(manifest.getActions()[1].name, "move");
}

TEST(Manifest, IgnoresComponentsThatAreNotReplicated) {
    World world;
    addComponent(world, "game.Local", {"value"}, ReplicationMode::kNone);
    addComponent(world, "game.Shared");
    const InputLayout inputs;

    const Manifest manifest = Manifest::build(world, inputs);

    ASSERT_EQ(manifest.getComponentEntries().size(), 1U);
    EXPECT_EQ(manifest.getComponentEntries()[0].name, "game.Shared");
}

TEST(Manifest, TranslatesShortIdsAndLocalIdsBothWays) {
    World world;
    const ComponentId zebra = addComponent(world, "game.Zebra");
    const ComponentId apple = addComponent(world, "game.Apple");
    const InputLayout inputs;

    const Manifest manifest = Manifest::build(world, inputs);

    EXPECT_EQ(manifest.toLocal(0), apple);
    EXPECT_EQ(manifest.toLocal(1), zebra);
    EXPECT_EQ(manifest.toLocal(2), std::nullopt);
    EXPECT_EQ(manifest.toShort(apple), 0);
    EXPECT_EQ(manifest.toShort(zebra), 1);
    EXPECT_EQ(manifest.toShort(99), std::nullopt);
    ASSERT_EQ(manifest.getComponents().size(), 2U);
    EXPECT_EQ(manifest.getComponents()[0], apple);
}

TEST(Manifest, HashDependsOnLayoutNamesAndActions) {
    TwoWorlds first;
    TwoWorlds same;
    const std::uint64_t reference = Manifest::build(first.server, first.serverInputs).getHash();
    EXPECT_EQ(Manifest::build(same.client, same.clientInputs).getHash(), reference);

    TwoWorlds otherLayout;
    addComponent(otherLayout.server, "game.Extra");
    EXPECT_NE(Manifest::build(otherLayout.server, otherLayout.serverInputs).getHash(), reference);

    TwoWorlds otherAction;
    otherAction.serverInputs.add("jump", InputKind::kButton);
    EXPECT_NE(Manifest::build(otherAction.server, otherAction.serverInputs).getHash(), reference);
}

TEST(Manifest, RefusesAReplicatedComponentThatCannotBeSerialized) {
    World world;
    ComponentInfo info;
    info.type.name = "game.Text";
    info.type.size = sizeof(float);
    info.type.alignment = alignof(float);
    info.type.replication = ReplicationMode::kEveryChange;
    info.type.fields = {{.name = "value", .type = FieldType::kF32, .offset = 0, .count = 1}};
    info.type.hooks.destruct = [](void*) noexcept {};
    info.type.hooks.move = [](void*, void*) noexcept {};
    info.type.hooks.copy = [](void*, const void*) {};
    ASSERT_TRUE(world.getComponents().registerType(std::move(info)).has_value());
    const InputLayout inputs;

    try {
        static_cast<void>(Manifest::build(world, inputs));
        FAIL() << "expected a ManifestException";
    } catch (const ManifestException& exception) {
        EXPECT_NE(std::string_view{exception.what()}.find("game.Text"), std::string_view::npos);
    }
}

TEST(Manifest, RefusesANameLongerThanTheWireAllows) {
    World world;
    addComponent(world, std::string(256, 'a'));
    const InputLayout inputs;

    EXPECT_THROW(static_cast<void>(Manifest::build(world, inputs)), ManifestException);
}

TEST(Manifest, EncodesOneMessagePerEntryThenTheEnd) {
    TwoWorlds setup;
    ReplicationSettings settings;
    settings.tickRate = 30;
    settings.snapshotRate = 10;
    settings.reconnectionGraceSeconds = 5;

    const std::vector<Bytes> messages = welcome(setup, settings);

    ASSERT_EQ(messages.size(), 6U);  // 3 components, 2 actions, the end
    EXPECT_EQ(ManifestCodec::peekKind(messages[0]), MessageType::kManifestComponent);
    EXPECT_EQ(ManifestCodec::peekKind(messages[3]), MessageType::kManifestInputAction);
    EXPECT_EQ(ManifestCodec::peekKind(messages[5]), MessageType::kManifestEnd);
    const auto end = ManifestCodec::decode(messages[5]);
    ASSERT_TRUE(end);
    const auto& message = std::get<ManifestEnd>(*end);
    EXPECT_EQ(message.entryCount, 5);
    EXPECT_EQ(message.hash, Manifest::build(setup.server, setup.serverInputs).getHash());
    EXPECT_EQ(message.tickRate, 30);
    EXPECT_EQ(message.snapshotRate, 10);
    EXPECT_EQ(message.reconnectionGraceSeconds, 5);
}

TEST(ManifestReceiver, AcceptsTheManifestAndChecksItOnAWorldRegisteredInAnotherOrder) {
    TwoWorlds setup;
    const Manifest server = Manifest::build(setup.server, setup.serverInputs);
    ManifestReceiver receiver;

    EXPECT_FALSE(receiver.isComplete());
    ASSERT_TRUE(feed(receiver, welcome(setup)));
    EXPECT_TRUE(receiver.isComplete());

    const auto check = receiver.check(setup.client, setup.clientInputs);
    EXPECT_TRUE(check.mismatch.empty());
    ASSERT_TRUE(check.manifest);
    EXPECT_EQ(check.manifest->getHash(), server.getHash());
    ASSERT_EQ(check.manifest->getComponentEntries().size(), server.getComponentEntries().size());
    for (std::uint16_t shortId = 0; shortId < server.getComponentEntries().size(); ++shortId) {
        const auto local = check.manifest->toLocal(shortId);
        ASSERT_TRUE(local);
        const auto* info = setup.client.getComponents().getRegistry().findById(*local);
        ASSERT_NE(info, nullptr);
        EXPECT_EQ(info->type.name, server.getComponentEntries()[shortId].name);
    }
}

TEST(ManifestReceiver, KeepsTheServerSettings) {
    TwoWorlds setup;
    ReplicationSettings settings;
    settings.tickRate = 120;
    settings.snapshotRate = 40;
    settings.reconnectionGraceSeconds = 600;
    ManifestReceiver receiver;

    ASSERT_TRUE(feed(receiver, welcome(setup, settings)));

    EXPECT_EQ(receiver.getServerSettings(), settings);
}

TEST(ManifestReceiver, NamesAComponentThatTheClientLacks) {
    TwoWorlds setup;
    addComponent(setup.server, "game.Shield");
    ManifestReceiver receiver;
    ASSERT_TRUE(feed(receiver, welcome(setup)));

    const auto check = receiver.check(setup.client, setup.clientInputs);

    EXPECT_FALSE(check.manifest);
    EXPECT_EQ(check.mismatch, "`game.Shield` exists on the server but not on the client: update your game");
}

TEST(ManifestReceiver, NamesAnExtraComponentOfTheClient) {
    TwoWorlds setup;
    addComponent(setup.client, "game.Cheat");
    ManifestReceiver receiver;
    ASSERT_TRUE(feed(receiver, welcome(setup)));

    const auto check = receiver.check(setup.client, setup.clientInputs);

    EXPECT_FALSE(check.manifest);
    EXPECT_EQ(check.mismatch, "`game.Cheat` exists on the client but not on the server: update your game");
}

TEST(ManifestReceiver, NamesAComponentWithAnotherLayout) {
    TwoWorlds setup;
    World client;
    addComponent(client, "game.Ammo");
    addComponent(client, "game.Position", {"x", "y", "z"});
    addComponent(client, "game.Health");
    ManifestReceiver receiver;
    ASSERT_TRUE(feed(receiver, welcome(setup)));

    const auto check = receiver.check(client, setup.clientInputs);

    EXPECT_FALSE(check.manifest);
    EXPECT_EQ(check.mismatch, "`game.Position` differs between client and server: update your game");
}

TEST(ManifestReceiver, ReportsTheFirstDifferenceInSortedOrder) {
    TwoWorlds setup;
    World client;
    addComponent(client, "game.Ammo", {"other"});
    addComponent(client, "game.Position", {"x", "y", "z"});
    addComponent(client, "game.Health");
    ManifestReceiver receiver;
    ASSERT_TRUE(feed(receiver, welcome(setup)));

    EXPECT_EQ(receiver.check(client, setup.clientInputs).mismatch,
              "`game.Ammo` differs between client and server: update your game");
}

TEST(ManifestReceiver, NamesAnInputActionOfAnotherKind) {
    TwoWorlds setup;
    InputLayout client;
    client.add("fire", InputKind::kAxis);
    client.add("move", InputKind::kVector2);
    ManifestReceiver receiver;
    ASSERT_TRUE(feed(receiver, welcome(setup)));

    const auto check = receiver.check(setup.client, client);

    EXPECT_FALSE(check.manifest);
    EXPECT_EQ(check.mismatch, "The input action `fire` differs between client and server: update your game");
}

TEST(ManifestReceiver, NamesAMissingAndAnExtraInputAction) {
    TwoWorlds setup;
    ManifestReceiver receiver;
    ASSERT_TRUE(feed(receiver, welcome(setup)));

    InputLayout missing;
    missing.add("fire", InputKind::kButton);
    EXPECT_EQ(receiver.check(setup.client, missing).mismatch,
              "The input action `move` exists on the server but not on the client: update your game");

    InputLayout extra;
    extra.add("dash", InputKind::kButton);
    extra.add("fire", InputKind::kButton);
    extra.add("move", InputKind::kVector2);
    EXPECT_EQ(receiver.check(setup.client, extra).mismatch,
              "The input action `dash` exists on the client but not on the server: update your game");
}

TEST(ManifestReceiver, ReportsAClientThatCannotBuildItsManifest) {
    TwoWorlds setup;
    ManifestReceiver receiver;
    ASSERT_TRUE(feed(receiver, welcome(setup)));
    World client;
    addComponent(client, std::string(300, 'a'));

    const auto check = receiver.check(client, setup.clientInputs);

    EXPECT_FALSE(check.manifest);
    EXPECT_FALSE(check.mismatch.empty());
}

TEST(ManifestReceiver, RefusesToCheckAnIncompleteManifest) {
    TwoWorlds setup;
    const std::vector<Bytes> messages = welcome(setup);
    ManifestReceiver receiver;
    ASSERT_TRUE(receiver.receive(messages.front()));

    const auto check = receiver.check(setup.client, setup.clientInputs);

    EXPECT_FALSE(receiver.isComplete());
    EXPECT_FALSE(check.manifest);
    EXPECT_FALSE(check.mismatch.empty());
}

TEST(ManifestReceiver, RejectsMessagesThatDoNotDecode) {
    TwoWorlds setup;
    std::vector<Bytes> messages = welcome(setup);
    ManifestReceiver receiver;

    EXPECT_FALSE(receiver.receive(std::span<const std::byte>{}));
    EXPECT_FALSE(receiver.receive(Bytes{std::byte{0xFF}}));
    Bytes truncated = messages.front();
    truncated.pop_back();
    EXPECT_FALSE(receiver.receive(truncated));
    Bytes longer = messages.front();
    longer.push_back(std::byte{0});
    EXPECT_FALSE(receiver.receive(longer));
    Bytes invalidKind = ManifestCodec::encode(ManifestInputAction{.index = 0, .name = "x", .kind = InputKind::kButton});
    invalidKind.back() = std::byte{0};
    EXPECT_FALSE(receiver.receive(invalidKind));
    EXPECT_FALSE(receiver.isComplete());
}

TEST(ManifestReceiver, RejectsADuplicatedShortIdOrIndex) {
    ManifestReceiver receiver;
    const Bytes component = ManifestCodec::encode(ManifestComponent{.shortId = 0, .name = "game.A", .layoutHash = 1});
    const Bytes action =
        ManifestCodec::encode(ManifestInputAction{.index = 0, .name = "fire", .kind = InputKind::kButton});

    ASSERT_TRUE(receiver.receive(component));
    EXPECT_FALSE(receiver.receive(ManifestCodec::encode(ManifestComponent{.shortId = 0, .name = "game.B"})));
    ASSERT_TRUE(receiver.receive(action));
    EXPECT_FALSE(receiver.receive(ManifestCodec::encode(ManifestInputAction{.index = 0, .name = "jump"})));
}

TEST(ManifestReceiver, RejectsAnEntryAfterTheEnd) {
    TwoWorlds setup;
    const std::vector<Bytes> messages = welcome(setup);
    ManifestReceiver receiver;
    ASSERT_TRUE(feed(receiver, messages));

    EXPECT_FALSE(receiver.receive(messages.front()));
    EXPECT_FALSE(receiver.receive(messages.back()));
    EXPECT_TRUE(receiver.isComplete());
}

TEST(ManifestReceiver, RejectsAnEndWithTheWrongCount) {
    ManifestReceiver receiver;
    ASSERT_TRUE(receiver.receive(ManifestCodec::encode(ManifestComponent{.shortId = 0, .name = "game.A"})));

    EXPECT_FALSE(receiver.receive(ManifestCodec::encode(ManifestEnd{.entryCount = 2})));
    EXPECT_FALSE(receiver.isComplete());
}

TEST(ManifestReceiver, RejectsEntriesWithGapsOrOutOfOrder) {
    ManifestReceiver gap;
    ASSERT_TRUE(gap.receive(ManifestCodec::encode(ManifestComponent{.shortId = 1, .name = "game.A"})));
    EXPECT_FALSE(gap.receive(ManifestCodec::encode(ManifestEnd{.entryCount = 1})));

    ManifestReceiver unsorted;
    ASSERT_TRUE(unsorted.receive(ManifestCodec::encode(ManifestComponent{.shortId = 0, .name = "game.B"})));
    ASSERT_TRUE(unsorted.receive(ManifestCodec::encode(ManifestComponent{.shortId = 1, .name = "game.A"})));
    EXPECT_FALSE(unsorted.receive(ManifestCodec::encode(ManifestEnd{.entryCount = 2})));
}

TEST(ManifestReceiver, AcceptsAnEmptyManifest) {
    World world;
    const InputLayout inputs;
    ManifestReceiver receiver;

    ASSERT_TRUE(feed(receiver, Manifest::build(world, inputs).encode({})));

    EXPECT_TRUE(receiver.isComplete());
    EXPECT_TRUE(receiver.check(world, inputs).manifest);
}

TEST(InputLayout, KeepsActionsOrderedByName) {
    InputLayout layout;
    layout.add("move", InputKind::kVector2);
    layout.add("fire", InputKind::kButton);
    layout.add("aim", InputKind::kAxis);

    ASSERT_EQ(layout.getActions().size(), 3U);
    EXPECT_EQ(layout.getActions()[0].name, "aim");
    EXPECT_EQ(layout.getActions()[1].name, "fire");
    EXPECT_EQ(layout.getActions()[2].name, "move");
}

TEST(InputLayout, RefusesDuplicatesInvalidNamesAndChangesOnceFrozen) {
    InputLayout layout;
    layout.add("fire", InputKind::kButton);

    EXPECT_THROW(layout.add("fire", InputKind::kAxis), InputLayoutException);
    EXPECT_THROW(layout.add("", InputKind::kButton), InputLayoutException);
    EXPECT_THROW(layout.add(std::string(256, 'a'), InputKind::kButton), InputLayoutException);
    EXPECT_FALSE(layout.isFrozen());
    layout.freeze();
    EXPECT_TRUE(layout.isFrozen());
    EXPECT_THROW(layout.add("jump", InputKind::kButton), InputLayoutException);
    EXPECT_EQ(layout.getActions().size(), 1U);
}
