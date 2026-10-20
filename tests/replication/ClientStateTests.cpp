/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ClientStateTests
*/

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "ServerFixtures.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/world/World.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/bytes/ByteWriter.hpp"
#include "rtype/network/client/ClientEvent.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/replication/MessageType.hpp"
#include "rtype/replication/ReplicationSettings.hpp"
#include "rtype/replication/client/ClientState.hpp"
#include "rtype/replication/component/NetworkId.hpp"
#include "rtype/replication/component/Owner.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/input/InputKind.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/input/InputMessage.hpp"
#include "rtype/replication/manifest/Manifest.hpp"
#include "rtype/replication/snapshot/Snapshot.hpp"

namespace {
namespace snapshot = rtype::replication::snapshot;
namespace network = rtype::network;
using rtype::ecs::Commands;
using rtype::ecs::Entity;
using rtype::ecs::Presence;
using rtype::ecs::ReplicationMode;
using rtype::ecs::World;
using rtype::replication::MessageType;
using rtype::replication::ReplicationSettings;
using rtype::replication::client::ClientState;
using rtype::replication::client::ConnectionStatus;
using rtype::replication::client::ManifestStatus;
using rtype::replication::component::NetworkId;
using rtype::replication::component::Owner;
using rtype::replication::component::PlayerInput;
using rtype::replication::input::InputKind;
using rtype::replication::input::InputLayout;
using rtype::replication::manifest::Manifest;
using rtype::test::server::Follow;
using rtype::test::server::Position;

constexpr ReplicationSettings kServerSettings{.tickRate = 30, .snapshotRate = 15, .reconnectionGraceSeconds = 7};

/// @brief Registers the components both sides of the tests have.
void registerComponents(World& world, const bool withFollow = true) {
    auto& components = world.getComponents();
    static_cast<void>(
        components.registerType<NetworkId>({.presence = Presence::kBoth, .replication = ReplicationMode::kNone}));
    static_cast<void>(
        components.registerType<Owner>({.presence = Presence::kBoth, .replication = ReplicationMode::kEveryChange}));
    static_cast<void>(
        components.registerType<Position>({.presence = Presence::kBoth, .replication = ReplicationMode::kEveryChange}));
    if (withFollow) {
        static_cast<void>(components.registerType<Follow>(
            {.presence = Presence::kBoth, .replication = ReplicationMode::kEveryChange}));
    }
}

/// @return The layout both sides share.
InputLayout makeLayout() {
    InputLayout layout;
    layout.add("fire", InputKind::kButton);
    layout.add("move", InputKind::kAxis);
    return layout;
}

/// @return The event of a message of the server.
network::client::ClientEvent serverMessage(network::Bytes payload) {
    return network::client::ServerMessage{.channel = network::Channel::kUnreliable, .payload = std::move(payload)};
}

/// @return The wire bytes of a Position.
network::Bytes positionBytes(const float x, const float y) {
    network::Bytes bytes(2 * sizeof(float));
    network::bytes::ByteWriter writer{bytes};
    writer.writeF32(x);
    writer.writeF32(y);
    return bytes;
}

/// @return The wire bytes of a u32 (a Follow, an Owner).
network::Bytes u32Bytes(const std::uint32_t value) {
    network::Bytes bytes(sizeof(std::uint32_t));
    network::bytes::ByteWriter writer{bytes};
    writer.writeU32(value);
    return bytes;
}

/// @brief A client state in a world that has the components of the server, fed with events by hand: no App, no socket.
class ClientStateTest : public ::testing::Test {
  protected:
    ClientStateTest()
        : _address{network::io::Address::resolve("127.0.0.1", 9).value()},
          _state{_address, network::NetworkSettings{}, _world} {
        registerComponents(_world);
        _layout = makeLayout();
        _positionId = _world.getComponents().find("test.server.Position").value();
        _followId = _world.getComponents().find("test.server.Follow").value();
        _ownerId = _world.getComponents().find("net.Owner").value();
    }

    /// @brief Gives the state the manifest of the world.
    void accept() {
        _layout.freeze();
        const Manifest manifest = Manifest::build(_world, _layout);
        for (network::Bytes& message : manifest.encode(kServerSettings)) {
            _state.handle(serverMessage(std::move(message)), _layout, _commands);
        }
    }

    /// @return The short id of a component of the manifest.
    [[nodiscard]] std::uint16_t shortId(const rtype::ecs::ComponentId id) const {
        return Manifest::build(_world, _layout).toShort(id).value();
    }

    [[nodiscard]] snapshot::ComponentValue position(const float x, const float y) const {
        return {.shortId = shortId(_positionId), .bytes = positionBytes(x, y)};
    }

    [[nodiscard]] snapshot::ComponentValue follow(const std::uint32_t target) const {
        return {.shortId = shortId(_followId), .bytes = u32Bytes(target)};
    }

    [[nodiscard]] snapshot::ComponentValue owner(const std::uint32_t client) const {
        return {.shortId = shortId(_ownerId), .bytes = u32Bytes(client)};
    }

    /// @brief Applies a snapshot given as a structure, then flushes the commands.
    bool apply(const snapshot::Snapshot& body) {
        const bool applied = _state.applySnapshot(snapshot::encode(body).value(), _commands);
        _commands.flush();
        return applied;
    }

    /// @return The Position of an entity, if it has one.
    [[nodiscard]] const Position* positionOf(const Entity entity) const {
        return _world.getComponents().get<Position>(entity);
    }

    /// @return The number of entities with a NetworkId.
    [[nodiscard]] std::size_t countTracked() const {
        std::size_t count = 0;
        for (std::uint32_t id = 1; id <= 20; ++id) {
            count += _state.find(id) == Entity{} ? 0 : 1;
        }
        return count;
    }

    World _world;
    InputLayout _layout;
    network::io::Address _address;
    Commands _commands{_world};
    ClientState _state;
    rtype::ecs::ComponentId _positionId{};
    rtype::ecs::ComponentId _followId{};
    rtype::ecs::ComponentId _ownerId{};
};
}  // namespace

TEST_F(ClientStateTest, StartsWaitingForTheManifest) {
    EXPECT_EQ(_state.manifestStatus(), ManifestStatus::kWaiting);
    EXPECT_EQ(_state.connectionStatus(), ConnectionStatus::kIdle);
    EXPECT_TRUE(_state.mismatch().empty());
    EXPECT_EQ(_state.find(1), Entity{});
    EXPECT_FALSE(_state.localPlayer());
}

TEST_F(ClientStateTest, AcceptsTheManifestOfTheSameWorld) {
    accept();
    EXPECT_EQ(_state.manifestStatus(), ManifestStatus::kAccepted);
    EXPECT_TRUE(_state.mismatch().empty());
    EXPECT_EQ(_state.serverSettings(), kServerSettings);
    EXPECT_TRUE(_layout.isFrozen());
}

TEST_F(ClientStateTest, RefusesTheManifestOfAnotherWorld) {
    World other;
    registerComponents(other, false);
    InputLayout layout = makeLayout();
    layout.freeze();
    for (network::Bytes& message : Manifest::build(other, layout).encode(kServerSettings)) {
        _state.handle(serverMessage(std::move(message)), _layout, _commands);
    }
    EXPECT_EQ(_state.manifestStatus(), ManifestStatus::kMismatch);
    EXPECT_NE(_state.mismatch().find("test.server.Follow"), std::string_view::npos);
}

TEST_F(ClientStateTest, DropsAMalformedManifestMessage) {
    _state.handle(serverMessage({static_cast<std::byte>(MessageType::kManifestComponent)}), _layout, _commands);
    _state.handle(serverMessage({}), _layout, _commands);
    _state.handle(serverMessage({static_cast<std::byte>(0x7F)}), _layout, _commands);
    EXPECT_EQ(_state.droppedCount(), 3U);
    EXPECT_EQ(_state.manifestStatus(), ManifestStatus::kWaiting);
}

TEST_F(ClientStateTest, DropsSnapshotsBeforeTheManifestIsAccepted) {
    EXPECT_FALSE(apply({.tick = 1, .spawns = {{.networkId = 1, .components = {position(1.F, 2.F)}}}}));
    EXPECT_EQ(_state.find(1), Entity{});
    EXPECT_EQ(_state.staleCount(), 1U);
    EXPECT_EQ(_state.lastAppliedTick(), 0U);
}

TEST_F(ClientStateTest, IgnoresInputsBeforeTheManifestIsAccepted) {
    _state.setInput(PlayerInput{.buttons = 1});
    EXPECT_FALSE(_state.nextInputMessage(_layout));
}

TEST_F(ClientStateTest, AppliesASnapshotToAnEmptyWorld) {
    accept();
    ASSERT_TRUE(apply({.tick = 4,
                       .lastProcessedInput = 9,
                       .spawns = {{.networkId = 1, .components = {position(1.F, 2.F)}},
                                  {.networkId = 5, .components = {position(3.F, 4.F)}}}}));
    const Entity first = _state.find(1);
    const Entity second = _state.find(5);
    ASSERT_NE(first, Entity{});
    ASSERT_NE(second, Entity{});
    EXPECT_NE(first, second);
    ASSERT_NE(positionOf(first), nullptr);
    EXPECT_FLOAT_EQ(positionOf(first)->y, 2.F);
    EXPECT_FLOAT_EQ(positionOf(second)->x, 3.F);
    EXPECT_EQ(_world.getComponents().get<NetworkId>(second)->value, 5U);
    EXPECT_EQ(_state.lastAppliedTick(), 4U);
    EXPECT_EQ(_state.lastProcessedInput(), 9U);
}

TEST_F(ClientStateTest, DropsASnapshotThatIsNotNewer) {
    accept();
    ASSERT_TRUE(apply({.tick = 5, .spawns = {{.networkId = 1, .components = {position(1.F, 1.F)}}}}));
    EXPECT_FALSE(apply({.tick = 5, .updates = {{.networkId = 1, .components = {position(9.F, 9.F)}}}}));
    EXPECT_FALSE(apply({.tick = 3, .updates = {{.networkId = 1, .components = {position(9.F, 9.F)}}}}));
    EXPECT_FLOAT_EQ(positionOf(_state.find(1))->x, 1.F);
    EXPECT_EQ(_state.lastAppliedTick(), 5U);
    EXPECT_EQ(_state.staleCount(), 2U);
}

TEST_F(ClientStateTest, LetsTwoEntitiesOfTheSameSnapshotReferToEachOther) {
    accept();
    ASSERT_TRUE(
        apply({.tick = 1,
               .spawns = {{.networkId = 1, .components = {follow(2)}}, {.networkId = 2, .components = {follow(1)}}}}));
    const Entity first = _state.find(1);
    const Entity second = _state.find(2);
    ASSERT_NE(first, Entity{});
    ASSERT_NE(second, Entity{});
    EXPECT_EQ(_world.getComponents().get<Follow>(first)->target, second);
    EXPECT_EQ(_world.getComponents().get<Follow>(second)->target, first);
}

TEST_F(ClientStateTest, ReadsAnUnknownReferenceAsTheNullEntity) {
    accept();
    ASSERT_TRUE(apply({.tick = 1, .spawns = {{.networkId = 1, .components = {follow(77)}}}}));
    EXPECT_EQ(_world.getComponents().get<Follow>(_state.find(1))->target, Entity{});
}

TEST_F(ClientStateTest, OverwritesComponentsOfKnownEntitiesAndAddsMissingOnes) {
    accept();
    ASSERT_TRUE(apply({.tick = 1, .spawns = {{.networkId = 1, .components = {position(1.F, 1.F)}}}}));
    ASSERT_TRUE(apply({.tick = 2,
                       .updates = {{.networkId = 1, .components = {position(7.F, 8.F), follow(1)}},
                                   {.networkId = 99, .components = {position(1.F, 1.F)}}}}));
    const Entity entity = _state.find(1);
    EXPECT_FLOAT_EQ(positionOf(entity)->x, 7.F);
    EXPECT_EQ(_world.getComponents().get<Follow>(entity)->target, entity);
    EXPECT_EQ(_state.find(99), Entity{});
}

TEST_F(ClientStateTest, RespawnsAnEntityTheClientDestroyed) {
    accept();
    ASSERT_TRUE(apply({.tick = 1, .spawns = {{.networkId = 1, .components = {position(1.F, 1.F)}}}}));
    const Entity first = _state.find(1);
    ASSERT_TRUE(_world.despawn(first));
    ASSERT_TRUE(apply({.tick = 2, .spawns = {{.networkId = 1, .components = {position(2.F, 2.F)}}}}));
    EXPECT_NE(_state.find(1), first);
    EXPECT_FLOAT_EQ(positionOf(_state.find(1))->x, 2.F);
}

TEST_F(ClientStateTest, RemovesComponentsAndDespawnsEntities) {
    accept();
    ASSERT_TRUE(apply({.tick = 1,
                       .spawns = {{.networkId = 1, .components = {position(1.F, 1.F), follow(2)}},
                                  {.networkId = 2, .components = {position(2.F, 2.F)}}}}));
    const Entity first = _state.find(1);
    const Entity second = _state.find(2);
    ASSERT_TRUE(apply({.tick = 2,
                       .removals = {{.networkId = 1, .shortId = shortId(_followId)},
                                    {.networkId = 42, .shortId = shortId(_followId)}},
                       .despawns = {2, 43}}));
    EXPECT_TRUE(_world.getComponents().has<Position>(first));
    EXPECT_FALSE(_world.getComponents().has<Follow>(first));
    EXPECT_FALSE(_world.isAlive(second));
    EXPECT_EQ(_state.find(2), Entity{});
    EXPECT_EQ(_state.find(1), first);
}

TEST_F(ClientStateTest, ACompleteSnapshotRemovesWhatItDoesNotList) {
    accept();
    ASSERT_TRUE(apply({.tick = 1,
                       .spawns = {{.networkId = 1, .components = {position(1.F, 1.F), follow(1)}},
                                  {.networkId = 2, .components = {position(2.F, 2.F)}}}}));
    const Entity first = _state.find(1);
    const Entity second = _state.find(2);
    ASSERT_TRUE(apply(
        {.tick = 2, .flags = snapshot::kComplete, .spawns = {{.networkId = 1, .components = {position(3.F, 3.F)}}}}));
    EXPECT_FALSE(_world.isAlive(second));
    EXPECT_EQ(_state.find(2), Entity{});
    EXPECT_TRUE(_world.getComponents().has<Position>(first));
    EXPECT_FALSE(_world.getComponents().has<Follow>(first));
    EXPECT_FLOAT_EQ(positionOf(first)->x, 3.F);
}

TEST_F(ClientStateTest, AnIncompleteSnapshotKeepsWhatItDoesNotList) {
    accept();
    ASSERT_TRUE(apply({.tick = 1,
                       .spawns = {{.networkId = 1, .components = {position(1.F, 1.F), follow(1)}},
                                  {.networkId = 2, .components = {position(2.F, 2.F)}}}}));
    ASSERT_TRUE(apply({.tick = 2, .spawns = {{.networkId = 1, .components = {position(3.F, 3.F)}}}}));
    EXPECT_NE(_state.find(2), Entity{});
    EXPECT_TRUE(_world.getComponents().has<Follow>(_state.find(1)));
}

TEST_F(ClientStateTest, AMalformedSnapshotAtAnyTruncationChangesNothing) {
    accept();
    ASSERT_TRUE(apply({.tick = 1, .spawns = {{.networkId = 1, .components = {position(1.F, 1.F)}}}}));
    const network::Bytes whole = snapshot::encode({.tick = 2,
                                                   .flags = snapshot::kComplete,
                                                   .spawns = {{.networkId = 1, .components = {position(5.F, 5.F)}},
                                                              {.networkId = 2, .components = {follow(1)}}},
                                                   .updates = {{.networkId = 1, .components = {follow(2)}}},
                                                   .removals = {{.networkId = 1, .shortId = shortId(_followId)}},
                                                   .despawns = {7}})
                                     .value();
    for (std::size_t length = 0; length < whole.size(); ++length) {
        const network::Bytes truncated{whole.begin(), whole.begin() + static_cast<std::ptrdiff_t>(length)};
        EXPECT_FALSE(_state.applySnapshot(truncated, _commands)) << "length " << length;
        EXPECT_EQ(_commands.getCount(), 0U) << "length " << length;
    }
    EXPECT_EQ(_state.lastAppliedTick(), 1U);
    EXPECT_EQ(_state.find(2), Entity{});
    EXPECT_FLOAT_EQ(positionOf(_state.find(1))->x, 1.F);
}

TEST_F(ClientStateTest, ASnapshotWithAnInvalidValueChangesNothing) {
    accept();
    ASSERT_TRUE(apply({.tick = 1, .spawns = {{.networkId = 1, .components = {position(1.F, 1.F)}}}}));
    const float nan = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(
        _state.applySnapshot(snapshot::encode({.tick = 2,
                                               .spawns = {{.networkId = 2, .components = {position(1.F, 1.F)}}},
                                               .updates = {{.networkId = 1, .components = {position(nan, 0.F)}}}})
                                 .value(),
                             _commands));
    EXPECT_EQ(_commands.getCount(), 0U);
    EXPECT_EQ(_state.find(2), Entity{});
    EXPECT_EQ(_state.lastAppliedTick(), 1U);
    EXPECT_FALSE(_state.applySnapshot({}, _commands));
    EXPECT_FALSE(_state.applySnapshot(network::Bytes{static_cast<std::byte>(MessageType::kSnapshot)}, _commands));
}

TEST_F(ClientStateTest, SendsTheLastThreeInputsAndTheLastAppliedTick) {
    accept();
    ASSERT_TRUE(apply({.tick = 12}));
    PlayerInput input{.buttons = 1};
    input.axes[0] = 0.5F;
    _state.setInput(input);
    std::vector<std::uint32_t> sequences;
    for (int frame = 1; frame <= 4; ++frame) {
        const auto message = rtype::replication::input::decodeInput(_state.nextInputMessage(_layout).value(), _layout);
        ASSERT_TRUE(message);
        EXPECT_EQ(message->lastAppliedSnapshot, 12U);
        sequences.clear();
        for (const auto& sample : message->inputs) {
            sequences.push_back(sample.sequence);
            EXPECT_EQ(sample.buttons, 1U);
            EXPECT_NEAR(sample.axes[0], 0.5F, 0.001F);
        }
        const std::size_t expected = frame < 3 ? static_cast<std::size_t>(frame) : 3;
        ASSERT_EQ(sequences.size(), expected);
        EXPECT_EQ(sequences.back(), static_cast<std::uint32_t>(frame));
        EXPECT_EQ(sequences.front(), static_cast<std::uint32_t>(frame) - static_cast<std::uint32_t>(expected) + 1);
    }
}

TEST_F(ClientStateTest, SendsTheNeutralInputWhenTheGameGaveNone) {
    accept();
    const auto message = rtype::replication::input::decodeInput(_state.nextInputMessage(_layout).value(), _layout);
    ASSERT_TRUE(message);
    ASSERT_EQ(message->inputs.size(), 1U);
    EXPECT_EQ(message->inputs.front().buttons, 0U);
}

TEST_F(ClientStateTest, FindsTheLocalPlayerByItsOwner) {
    accept();
    _state.handle(network::client::ConnectionAccepted{.client = 7}, _layout, _commands);
    EXPECT_EQ(_state.connectionStatus(), ConnectionStatus::kConnected);
    EXPECT_EQ(_state.clientId(), 7U);
    ASSERT_TRUE(apply({.tick = 1,
                       .spawns = {{.networkId = 1, .components = {owner(3)}},
                                  {.networkId = 2, .components = {owner(7)}},
                                  {.networkId = 3, .components = {position(0.F, 0.F)}}}}));
    EXPECT_EQ(_state.localPlayer(), _state.find(2));
    ASSERT_TRUE(apply({.tick = 2, .despawns = {2}}));
    EXPECT_FALSE(_state.localPlayer());
}

TEST_F(ClientStateTest, RecordsTheEndOfTheConnection) {
    _state.handle(network::client::Disconnected{.reason = network::DisconnectReason::kKicked}, _layout, _commands);
    EXPECT_EQ(_state.connectionStatus(), ConnectionStatus::kDisconnected);
    EXPECT_EQ(_state.disconnectReason(), network::DisconnectReason::kKicked);
    _state.handle(network::client::ConnectFailed{.reason = network::RejectReason::kServerFull}, _layout, _commands);
    EXPECT_EQ(_state.connectionStatus(), ConnectionStatus::kFailed);
    EXPECT_EQ(_state.rejectReason(), network::RejectReason::kServerFull);
    _state.handle(network::client::ConnectFailed{.reason = std::nullopt}, _layout, _commands);
    EXPECT_FALSE(_state.rejectReason());
}
