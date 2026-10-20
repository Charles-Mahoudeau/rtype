/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ServerStateTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "ServerFixtures.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/DynamicQuery.hpp"
#include "rtype/ecs/query/QueryBuilder.hpp"
#include "rtype/ecs/system/DynamicQueries.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/bytes/ByteWriter.hpp"
#include "rtype/network/protocol/Constants.hpp"
#include "rtype/network/server/RoomMessages.hpp"
#include "rtype/replication/MessageType.hpp"
#include "rtype/replication/ReplicationPlugin.hpp"
#include "rtype/replication/ReplicationSettings.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/event/ClientConnected.hpp"
#include "rtype/replication/event/ClientDisconnected.hpp"
#include "rtype/replication/exceptions/ReplicationExceptions.hpp"
#include "rtype/replication/input/InputKind.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/input/InputMessage.hpp"
#include "rtype/replication/query/ReplicatedQueries.hpp"
#include "rtype/replication/server/ServerState.hpp"
#include "rtype/replication/snapshot/Snapshot.hpp"

namespace {
namespace snapshot = rtype::replication::snapshot;
namespace network = rtype::network;
using rtype::ecs::App;
using rtype::ecs::DynamicQuery;
using rtype::ecs::Entity;
using rtype::ecs::Presence;
using rtype::ecs::ReplicationMode;
using rtype::replication::ReplicationPlugin;
using rtype::replication::ReplicationSettings;
using rtype::replication::component::PlayerInput;
using rtype::replication::event::ClientConnected;
using rtype::replication::event::ClientDisconnected;
using rtype::replication::exceptions::ReplicationException;
using rtype::replication::input::InputKind;
using rtype::replication::input::InputLayout;
using rtype::replication::input::InputMessage;
using rtype::replication::input::InputSample;
using rtype::replication::server::ServerNotification;
using rtype::replication::server::ServerState;
using rtype::test::server::Follow;
using rtype::test::server::Position;
using rtype::test::server::Secret;

constexpr network::ClientId kFirst = 1;
constexpr network::ClientId kSecond = 2;

/// @brief A world with the replication types and two replicated game components, and a server state over it.
class ServerStateTest : public ::testing::Test {
  protected:
    void SetUp() override {
        _app.addPlugin(ReplicationPlugin{});
        auto& components = _app.getWorld().getComponents();
        static_cast<void>(components.registerType<Position>(
            {.presence = Presence::kBoth, .replication = ReplicationMode::kEveryChange}));
        static_cast<void>(components.registerType<Follow>(
            {.presence = Presence::kBoth, .replication = ReplicationMode::kEveryChange}));
        static_cast<void>(components.registerType<Secret>());
        _layout.add("fire", InputKind::kButton);
        _layout.add("jump", InputKind::kButton);
        _layout.add("move", InputKind::kAxis);
    }

    /// @brief Makes the state under test over the world, and prepares it.
    void makeState(const ReplicationSettings& settings = {}) {
        _state = std::make_unique<ServerState>(network::NetworkSettings{}, settings, world());
        _state->prepare(_layout);
    }

    [[nodiscard]] rtype::ecs::World& world() { return _app.getWorld(); }

    /// @brief Makes the connection of a client known.
    void join(const network::ClientId client) {
        const std::optional<ServerNotification> notification =
            _state->handle(network::server::ClientJoined{client}, _layout);
        ASSERT_TRUE(notification);
    }

    /// @brief Gives the state an input message from a client.
    void sendInput(const network::ClientId client, const std::vector<InputSample>& inputs,
                   const std::uint32_t lastAppliedSnapshot = 0) {
        const network::Bytes payload = rtype::replication::input::encodeInput(
            {.lastAppliedSnapshot = lastAppliedSnapshot, .inputs = inputs}, _layout);
        static_cast<void>(_state->handle(
            network::server::ClientMessage{
                .client = client, .channel = network::Channel::kUnreliable, .payload = payload},
            _layout));
    }

    /// @brief Spawns a replicated entity and gives it an id.
    Entity spawnTracked(const float x) {
        const Entity entity = world().spawn(Position{.x = x, .y = -x});
        EXPECT_TRUE(_state->track(entity));
        return entity;
    }

    /// @brief Builds the snapshots of the tick with the queries of the replicated components.
    std::vector<network::server::SendMessage> build(const std::uint32_t tick) {
        _queries.clear();
        for (const rtype::ecs::QueryBuilder& builder :
             rtype::replication::query::makeReplicatedReads(world()).queries) {
            _queries.push_back(builder.build(world().getComponents()).value());
        }
        return _state->buildSnapshots(tick, rtype::ecs::DynamicQueries{_queries, world().getChangeTick()});
    }

    /// @brief Decodes the payload of a snapshot command.
    [[nodiscard]] snapshot::Snapshot decode(const network::server::SendMessage& command) {
        const std::vector<std::size_t> sizes = snapshot::computeComponentSizes(_state->manifest(), world());
        const std::optional<snapshot::Snapshot> decoded = snapshot::decode(command.payload, sizes);
        EXPECT_TRUE(decoded);
        return decoded.value_or(snapshot::Snapshot{});
    }

    /// @return The short id of a component of the manifest.
    [[nodiscard]] std::uint16_t shortId(const std::string_view name) {
        const auto id = world().getComponents().find(name);
        return _state->manifest().toShort(id.value()).value();
    }

    App _app;
    InputLayout _layout;
    std::unique_ptr<ServerState> _state;
    std::vector<DynamicQuery> _queries;
};

/// @return A sample with one button held.
InputSample makeSample(const std::uint32_t sequence, const std::uint32_t buttons) {
    InputSample sample;
    sample.sequence = sequence;
    sample.buttons = buttons;
    return sample;
}

/// @return The bytes of a Position on the wire.
network::Bytes positionBytes(const float x, const float y) {
    network::Bytes bytes(8);
    network::bytes::ByteWriter writer{bytes};
    writer.writeF32(x);
    writer.writeF32(y);
    return bytes;
}

/// @return The bytes of an entity field on the wire.
network::Bytes idBytes(const std::uint32_t networkId) {
    network::Bytes bytes(4);
    network::bytes::ByteWriter writer{bytes};
    writer.writeU32(networkId);
    return bytes;
}
}  // namespace

TEST_F(ServerStateTest, StartsWithoutClientsNorEntities) {
    makeState();
    EXPECT_EQ(_state->clientCount(), 0U);
    EXPECT_EQ(_state->entityCount(), 0U);
    EXPECT_EQ(_state->serverTick(), 0U);
    EXPECT_FALSE(_state->isStarted());
}

TEST_F(ServerStateTest, PrepareFreezesTheInputLayoutAndBuildsTheManifest) {
    makeState();
    EXPECT_TRUE(_layout.isFrozen());
    // net.Owner, test.server.Follow and test.server.Position, by name.
    EXPECT_EQ(_state->manifest().components().size(), 3U);
    EXPECT_EQ(_state->manifest().actions().size(), 3U);
}

TEST_F(ServerStateTest, PrepareRefusesASnapshotRateThatDoesNotDivideTheTickRate) {
    _state = std::make_unique<ServerState>(network::NetworkSettings{},
                                           ReplicationSettings{.tickRate = 60, .snapshotRate = 7}, world());
    EXPECT_THROW(_state->prepare(_layout), ReplicationException);
}

TEST_F(ServerStateTest, PrepareRefusesARateOfZero) {
    _state = std::make_unique<ServerState>(network::NetworkSettings{},
                                           ReplicationSettings{.tickRate = 60, .snapshotRate = 0}, world());
    EXPECT_THROW(_state->prepare(_layout), ReplicationException);
    _state = std::make_unique<ServerState>(network::NetworkSettings{},
                                           ReplicationSettings{.tickRate = 0, .snapshotRate = 0}, world());
    EXPECT_THROW(_state->prepare(_layout), ReplicationException);
}

TEST_F(ServerStateTest, PrepareNamesAReplicatedComponentRegisteredAfterTheState) {
    _state = std::make_unique<ServerState>(network::NetworkSettings{}, ReplicationSettings{}, world());
    struct Late {
        float value{0.F};
    };
    (void)sizeof(Late);
    rtype::ecs::ComponentInfo info;
    info.type.name = "test.server.Late";
    info.type.size = sizeof(float);
    info.type.alignment = alignof(float);
    info.type.presence = Presence::kBoth;
    info.type.replication = ReplicationMode::kEveryChange;
    info.type.fields.push_back({.name = "value", .type = rtype::ecs::FieldType::kF32, .offset = 0, .count = 1});
    ASSERT_TRUE(world().getComponents().registerType(info));

    try {
        _state->prepare(_layout);
        FAIL() << "prepare() should refuse a component registered after the state";
    } catch (const ReplicationException& error) {
        EXPECT_NE(std::string{error.what()}.find("test.server.Late"), std::string::npos) << error.what();
    }
}

TEST_F(ServerStateTest, AClientJoiningIsAnnounced) {
    makeState();
    const std::optional<ServerNotification> notification =
        _state->handle(network::server::ClientJoined{kFirst}, _layout);
    ASSERT_TRUE(notification);
    const auto* connected = std::get_if<ClientConnected>(&*notification);
    ASSERT_NE(connected, nullptr);
    EXPECT_EQ(connected->client, kFirst);
    EXPECT_EQ(_state->clientCount(), 1U);
    EXPECT_TRUE(_state->hasClient(kFirst));
}

TEST_F(ServerStateTest, AClientLeavingIsAnnouncedWithItsReason) {
    makeState();
    join(kFirst);
    const std::optional<ServerNotification> notification = _state->handle(
        network::server::ClientLeft{.client = kFirst, .reason = network::DisconnectReason::kTimeout}, _layout);
    ASSERT_TRUE(notification);
    const auto* left = std::get_if<ClientDisconnected>(&*notification);
    ASSERT_NE(left, nullptr);
    EXPECT_EQ(left->client, kFirst);
    EXPECT_EQ(left->reason, static_cast<std::uint8_t>(network::DisconnectReason::kTimeout));
    EXPECT_EQ(_state->clientCount(), 0U);
}

TEST_F(ServerStateTest, AnUnknownClientLeavingIsNotAnnounced) {
    makeState();
    EXPECT_FALSE(_state->handle(
        network::server::ClientLeft{.client = 9, .reason = network::DisconnectReason::kGraceful}, _layout));
}

TEST_F(ServerStateTest, InputsReachTheBufferOfTheirClient) {
    makeState();
    join(kFirst);
    join(kSecond);
    sendInput(kFirst, {makeSample(1, 0b01)}, 40);
    sendInput(kSecond, {makeSample(5, 0b10)}, 50);

    PlayerInput first;
    PlayerInput second;
    _state->applyInput(kFirst, first);
    _state->applyInput(kSecond, second);

    EXPECT_EQ(first.buttons, 0b01U);
    EXPECT_EQ(first.sequence, 1U);
    EXPECT_EQ(second.buttons, 0b10U);
    EXPECT_EQ(second.sequence, 5U);
    EXPECT_EQ(_state->lastProcessedInput(kFirst), 1U);
    EXPECT_EQ(_state->lastProcessedInput(kSecond), 5U);
    EXPECT_EQ(_state->lastAppliedSnapshot(kFirst), 40U);
    EXPECT_EQ(_state->lastAppliedSnapshot(kSecond), 50U);
}

TEST_F(ServerStateTest, ApplyingAnInputKeepsThePreviousButtons) {
    makeState();
    join(kFirst);
    sendInput(kFirst, {makeSample(1, 0b01), makeSample(2, 0b10)});

    PlayerInput input;
    _state->applyInput(kFirst, input);
    EXPECT_EQ(input.previousButtons, 0U);
    EXPECT_EQ(input.buttons, 0b01U);
    _state->applyInput(kFirst, input);
    EXPECT_EQ(input.previousButtons, 0b01U);
    EXPECT_EQ(input.buttons, 0b10U);
}

TEST_F(ServerStateTest, TheLastInputIsRepeatedWhenNoneArrives) {
    makeState();
    join(kFirst);
    sendInput(kFirst, {makeSample(3, 0b11)});

    PlayerInput input;
    _state->applyInput(kFirst, input);
    _state->applyInput(kFirst, input);
    _state->applyInput(kFirst, input);

    EXPECT_EQ(input.buttons, 0b11U);
    EXPECT_EQ(input.previousButtons, 0b11U);
    EXPECT_EQ(input.sequence, 3U);
}

TEST_F(ServerStateTest, AnOwnerThatIsNotConnectedGetsTheNeutralInput) {
    makeState();
    PlayerInput input;
    input.buttons = 0b11;
    input.axes[0] = 0.5F;
    input.sequence = 8;

    _state->applyInput(kFirst, input);

    EXPECT_EQ(input.previousButtons, 0b11U);
    EXPECT_EQ(input.buttons, 0U);
    EXPECT_EQ(input.axes[0], 0.F);
    EXPECT_EQ(input.sequence, 0U);
}

TEST_F(ServerStateTest, AClientThatLeftGetsTheNeutralInput) {
    makeState();
    join(kFirst);
    sendInput(kFirst, {makeSample(1, 0b01)});
    PlayerInput input;
    _state->applyInput(kFirst, input);
    static_cast<void>(_state->handle(
        network::server::ClientLeft{.client = kFirst, .reason = network::DisconnectReason::kGraceful}, _layout));

    _state->applyInput(kFirst, input);

    EXPECT_EQ(input.buttons, 0U);
    EXPECT_EQ(input.previousButtons, 0b01U);
}

TEST_F(ServerStateTest, AnInputThatCannotBeDecodedIsDroppedAndCounted) {
    makeState();
    join(kFirst);
    static_cast<void>(_state->handle(
        network::server::ClientMessage{
            .client = kFirst, .channel = network::Channel::kUnreliable, .payload = {std::byte{0x31}, std::byte{0x00}}},
        _layout));

    EXPECT_EQ(_state->droppedInputCount(), 1U);
    PlayerInput input;
    _state->applyInput(kFirst, input);
    EXPECT_EQ(input.sequence, 0U);
}

TEST_F(ServerStateTest, OtherMessagesAreIgnoredAndCounted) {
    makeState();
    join(kFirst);
    EXPECT_FALSE(_state->handle(
        network::server::ClientMessage{
            .client = kFirst, .channel = network::Channel::kReliable, .payload = {std::byte{0x32}, std::byte{0x01}}},
        _layout));
    EXPECT_FALSE(_state->handle(
        network::server::ClientMessage{.client = kFirst, .channel = network::Channel::kReliable, .payload = {}},
        _layout));
    EXPECT_EQ(_state->ignoredMessageCount(), 2U);
    EXPECT_EQ(_state->droppedInputCount(), 0U);
}

TEST_F(ServerStateTest, AnInputFromAnUnknownClientIsIgnoredAndCounted) {
    makeState();
    const network::Bytes payload =
        rtype::replication::input::encodeInput({.lastAppliedSnapshot = 0, .inputs = {makeSample(1, 1)}}, _layout);
    EXPECT_FALSE(_state->handle(
        network::server::ClientMessage{.client = 7, .channel = network::Channel::kUnreliable, .payload = payload},
        _layout));
    EXPECT_EQ(_state->ignoredMessageCount(), 1U);
    EXPECT_EQ(_state->lastProcessedInput(7), 0U);
}

TEST_F(ServerStateTest, TheLastAppliedSnapshotNeverGoesBack) {
    makeState();
    join(kFirst);
    sendInput(kFirst, {makeSample(1, 1)}, 30);
    sendInput(kFirst, {makeSample(2, 1)}, 20);
    EXPECT_EQ(_state->lastAppliedSnapshot(kFirst), 30U);
}

TEST_F(ServerStateTest, TracksEntitiesWithIncreasingIds) {
    makeState();
    const Entity first = world().spawn(Position{});
    const Entity second = world().spawn(Position{});

    EXPECT_EQ(_state->track(first), 1U);
    EXPECT_EQ(_state->track(second), 2U);
    EXPECT_EQ(_state->findNetworkId(first), 1U);
    EXPECT_EQ(_state->findEntity(2U), second);
    EXPECT_EQ(_state->entityCount(), 2U);
}

TEST_F(ServerStateTest, TrackingAnEntityTwiceGivesNothing) {
    makeState();
    const Entity entity = world().spawn(Position{});
    ASSERT_TRUE(_state->track(entity));
    EXPECT_FALSE(_state->track(entity));
    EXPECT_EQ(_state->entityCount(), 1U);
}

TEST_F(ServerStateTest, AnUntrackedEntityHasNoId) {
    makeState();
    EXPECT_FALSE(_state->findNetworkId(world().spawn(Position{})));
    EXPECT_FALSE(_state->findEntity(1U));
}

TEST_F(ServerStateTest, NetworkIdsAreNeverReused) {
    makeState();
    join(kFirst);
    const Entity first = spawnTracked(1.F);
    world().despawn(first);
    static_cast<void>(build(1));  // forgets the dead entity
    EXPECT_FALSE(_state->findEntity(1U));

    const Entity second = world().spawn(Position{});
    EXPECT_EQ(_state->track(second), 2U);
}

TEST_F(ServerStateTest, ASnapshotIsDueEveryTickRateOverSnapshotRateTicks) {
    makeState({.tickRate = 60, .snapshotRate = 20});
    EXPECT_FALSE(_state->isSnapshotDue(0));
    EXPECT_FALSE(_state->isSnapshotDue(1));
    EXPECT_FALSE(_state->isSnapshotDue(2));
    EXPECT_TRUE(_state->isSnapshotDue(3));

    static_cast<void>(build(3));
    EXPECT_EQ(_state->serverTick(), 3U);
    EXPECT_FALSE(_state->isSnapshotDue(3));
    EXPECT_FALSE(_state->isSnapshotDue(5));
    EXPECT_TRUE(_state->isSnapshotDue(6));
}

TEST_F(ServerStateTest, ASnapshotIsDueWhenAFrameJumpsOverTheMultiple) {
    makeState({.tickRate = 60, .snapshotRate = 20});
    static_cast<void>(build(3));
    EXPECT_TRUE(_state->isSnapshotDue(8));
}

TEST_F(ServerStateTest, ASnapshotIsDueEveryTickAtTheTickRate) {
    makeState({.tickRate = 60, .snapshotRate = 60});
    EXPECT_FALSE(_state->isSnapshotDue(0));
    EXPECT_TRUE(_state->isSnapshotDue(1));
}

TEST_F(ServerStateTest, BuildsNothingWithoutClients) {
    makeState();
    static_cast<void>(spawnTracked(1.F));
    EXPECT_TRUE(build(1).empty());
}

TEST_F(ServerStateTest, BuildsOneUnreliableSnapshotPerClient) {
    makeState();
    join(kSecond);
    join(kFirst);

    const auto commands = build(4);

    ASSERT_EQ(commands.size(), 2U);
    EXPECT_EQ(commands[0].client, kFirst);
    EXPECT_EQ(commands[1].client, kSecond);
    EXPECT_EQ(commands[0].channel, network::Channel::kUnreliable);
    EXPECT_EQ(commands[0].payload.front(), static_cast<std::byte>(rtype::replication::MessageType::kSnapshot));
}

TEST_F(ServerStateTest, TheSnapshotListsEveryTrackedEntityInIdOrder) {
    makeState();
    join(kFirst);
    const Entity first = spawnTracked(1.F);
    const Entity second = spawnTracked(2.F);
    static_cast<void>(first);
    static_cast<void>(second);

    const auto commands = build(9);

    ASSERT_EQ(commands.size(), 1U);
    const snapshot::Snapshot decoded = decode(commands.front());
    EXPECT_EQ(decoded.tick, 9U);
    EXPECT_EQ(decoded.flags, snapshot::kComplete);
    ASSERT_EQ(decoded.spawns.size(), 2U);
    EXPECT_EQ(decoded.spawns[0].networkId, 1U);
    EXPECT_EQ(decoded.spawns[1].networkId, 2U);
    EXPECT_EQ(decoded.spawns[0].prefab, snapshot::kNoPrefab);
    ASSERT_EQ(decoded.spawns[0].components.size(), 1U);
    EXPECT_EQ(decoded.spawns[0].components[0].shortId, shortId("test.server.Position"));
    EXPECT_EQ(decoded.spawns[0].components[0].bytes, positionBytes(1.F, -1.F));
    EXPECT_EQ(decoded.spawns[1].components[0].bytes, positionBytes(2.F, -2.F));
    EXPECT_TRUE(decoded.updates.empty());
    EXPECT_TRUE(decoded.removals.empty());
    EXPECT_TRUE(decoded.despawns.empty());
}

TEST_F(ServerStateTest, TheComponentsOfARecordFollowTheShortIds) {
    makeState();
    join(kFirst);
    const Entity target = spawnTracked(1.F);
    const Entity follower = world().spawn(Follow{.target = target}, Position{.x = 5.F, .y = 6.F});
    ASSERT_TRUE(_state->track(follower));

    const snapshot::Snapshot decoded = decode(build(1).front());

    ASSERT_EQ(decoded.spawns.size(), 2U);
    const snapshot::SpawnRecord& record = decoded.spawns[1];
    ASSERT_EQ(record.components.size(), 2U);
    EXPECT_EQ(record.components[0].shortId, shortId("test.server.Follow"));
    EXPECT_EQ(record.components[1].shortId, shortId("test.server.Position"));
    EXPECT_LT(record.components[0].shortId, record.components[1].shortId);
    EXPECT_EQ(record.components[1].bytes, positionBytes(5.F, 6.F));
}

TEST_F(ServerStateTest, EntityFieldsAreWrittenAsNetworkIds) {
    makeState();
    join(kFirst);
    const Entity target = spawnTracked(1.F);
    const Entity follower = world().spawn(Follow{.target = target});
    ASSERT_TRUE(_state->track(follower));

    const snapshot::Snapshot decoded = decode(build(1).front());

    ASSERT_EQ(decoded.spawns.size(), 2U);
    EXPECT_EQ(decoded.spawns[1].components[0].bytes, idBytes(*_state->findNetworkId(target)));
}

TEST_F(ServerStateTest, AnEntityFieldThatIsNotTrackedIsWrittenAsZero) {
    makeState();
    join(kFirst);
    const Entity stranger = world().spawn(Position{});
    const Entity follower = world().spawn(Follow{.target = stranger});
    ASSERT_TRUE(_state->track(follower));

    const snapshot::Snapshot decoded = decode(build(1).front());

    ASSERT_EQ(decoded.spawns.size(), 1U);
    EXPECT_EQ(decoded.spawns[0].components[0].bytes, idBytes(0));
}

TEST_F(ServerStateTest, EntitiesNotTrackedYetAreNotListed) {
    makeState();
    join(kFirst);
    static_cast<void>(world().spawn(Position{}));
    EXPECT_TRUE(decode(build(1).front()).spawns.empty());
}

TEST_F(ServerStateTest, ComponentsThatAreNotReplicatedAreNotListed) {
    makeState();
    join(kFirst);
    const Entity entity = world().spawn(Position{.x = 1.F, .y = -1.F}, Secret{.value = 3.F});
    ASSERT_TRUE(_state->track(entity));

    const snapshot::Snapshot decoded = decode(build(1).front());

    ASSERT_EQ(decoded.spawns.size(), 1U);
    EXPECT_EQ(decoded.spawns[0].components.size(), 1U);
}

TEST_F(ServerStateTest, EachSnapshotCarriesTheLastInputOfItsClient) {
    makeState();
    join(kFirst);
    join(kSecond);
    sendInput(kFirst, {makeSample(4, 1)});
    PlayerInput input;
    _state->applyInput(kFirst, input);

    const auto commands = build(1);

    ASSERT_EQ(commands.size(), 2U);
    EXPECT_EQ(decode(commands[0]).lastProcessedInput, 4U);
    EXPECT_EQ(decode(commands[1]).lastProcessedInput, 0U);
}

TEST_F(ServerStateTest, DeadEntitiesDisappearFromTheNextSnapshot) {
    makeState();
    join(kFirst);
    const Entity first = spawnTracked(1.F);
    const Entity second = spawnTracked(2.F);
    ASSERT_EQ(decode(build(1).front()).spawns.size(), 2U);

    world().despawn(first);
    const snapshot::Snapshot decoded = decode(build(2).front());

    ASSERT_EQ(decoded.spawns.size(), 1U);
    EXPECT_EQ(decoded.spawns[0].networkId, *_state->findNetworkId(second));
    EXPECT_EQ(_state->entityCount(), 1U);
    EXPECT_FALSE(_state->findNetworkId(first));
}

TEST_F(ServerStateTest, DeadEntitiesAreForgottenEvenWithoutClients) {
    makeState();
    const Entity entity = spawnTracked(1.F);
    world().despawn(entity);
    EXPECT_TRUE(build(1).empty());
    EXPECT_EQ(_state->entityCount(), 0U);
}

TEST_F(ServerStateTest, TheSnapshotIsCompleteWhenEverythingFits) {
    makeState();
    join(kFirst);
    for (int index = 0; index < 20; ++index) {
        static_cast<void>(spawnTracked(static_cast<float>(index)));
    }
    const snapshot::Snapshot decoded = decode(build(1).front());
    EXPECT_EQ(decoded.spawns.size(), 20U);
    EXPECT_EQ(decoded.flags, snapshot::kComplete);
    EXPECT_EQ(_state->incompleteSnapshotCount(), 0U);
}

TEST_F(ServerStateTest, TheSnapshotIsNotCompleteAboveTheBudget) {
    makeState();
    join(kFirst);
    constexpr std::size_t kEntities = 200;  // 200 records of 19 bytes do not fit in 1000
    for (std::size_t index = 0; index < kEntities; ++index) {
        static_cast<void>(spawnTracked(static_cast<float>(index)));
    }

    const auto commands = build(1);

    ASSERT_EQ(commands.size(), 1U);
    EXPECT_LE(commands.front().payload.size(), network::protocol::kMaxMessageSize);
    const snapshot::Snapshot decoded = decode(commands.front());
    EXPECT_EQ(decoded.flags, 0U);
    EXPECT_GT(decoded.spawns.size(), 0U);
    EXPECT_LT(decoded.spawns.size(), kEntities);
    EXPECT_EQ(_state->incompleteSnapshotCount(), 1U);
    // The entities that fit come whole, lowest ids first.
    for (std::size_t index = 0; index < decoded.spawns.size(); ++index) {
        EXPECT_EQ(decoded.spawns[index].networkId, index + 1);
        EXPECT_EQ(decoded.spawns[index].components.size(), 1U);
    }
}
