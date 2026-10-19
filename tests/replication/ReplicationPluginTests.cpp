/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ReplicationPluginTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/event/EventInfo.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/exceptions/TypeExceptions.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"
#include "rtype/replication/ReplicationPlugin.hpp"
#include "rtype/replication/component/NetworkId.hpp"
#include "rtype/replication/component/Owner.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/event/ClientConnected.hpp"
#include "rtype/replication/event/ClientDisconnected.hpp"

namespace {
using rtype::ecs::App;
using rtype::ecs::ComponentInfo;
using rtype::ecs::EventCursor;
using rtype::ecs::EventInfo;
using rtype::ecs::FieldType;
using rtype::ecs::Presence;
using rtype::ecs::ReplicationMode;
using rtype::ecs::TypeTraits;
using rtype::replication::ReplicationPlugin;
using rtype::replication::component::kMaxInputAxes;
using rtype::replication::component::NetworkId;
using rtype::replication::component::Owner;
using rtype::replication::component::PlayerInput;
using rtype::replication::event::ClientConnected;
using rtype::replication::event::ClientDisconnected;

/// @return The registered description of the component with this name, or nullptr.
const ComponentInfo* findComponent(const App& app, std::string_view name) {
    return app.getWorld().getComponents().getRegistry().findByName(name);
}

/// @return The registered description of the event with this name, or nullptr.
const EventInfo* findEvent(const App& app, std::string_view name) {
    return app.getWorld().getEvents().getRegistry().findByName(name);
}

/// @return The names of the components the server sends to clients.
std::vector<std::string_view> replicatedNames(const App& app) {
    const auto& registry = app.getWorld().getComponents().getRegistry();
    std::vector<std::string_view> names;
    for (const rtype::ecs::TypeId id : registry.getReplicatedIds()) {
        names.emplace_back(registry.findById(id)->type.name);
    }
    return names;
}
}  // namespace

TEST(ReplicationPlugin, IsNamedRtypeReplication) {
    EXPECT_EQ(ReplicationPlugin{}.getName(), "rtype.replication");
    EXPECT_EQ(ReplicationPlugin::kName, "rtype.replication");
}

TEST(ReplicationPlugin, RegistersEveryTypeInAFreshApp) {
    App app;
    app.addPlugin(ReplicationPlugin{});

    EXPECT_TRUE(app.hasPlugin("rtype.replication"));
    EXPECT_NE(findComponent(app, "net.NetworkId"), nullptr);
    EXPECT_NE(findComponent(app, "net.Owner"), nullptr);
    EXPECT_NE(findComponent(app, "net.PlayerInput"), nullptr);
    EXPECT_NE(findEvent(app, "net.ClientConnected"), nullptr);
    EXPECT_NE(findEvent(app, "net.ClientDisconnected"), nullptr);
}

TEST(ReplicationPlugin, AddedTwiceDoesNothingTheSecondTime) {
    App app;
    app.addPlugin(ReplicationPlugin{});
    const std::size_t componentCount = app.getWorld().getComponents().getRegistry().getCount();
    const std::size_t eventCount = app.getWorld().getEvents().getRegistry().getCount();

    EXPECT_NO_THROW(app.addPlugin(ReplicationPlugin{}));
    EXPECT_EQ(app.getWorld().getComponents().getRegistry().getCount(), componentCount);
    EXPECT_EQ(app.getWorld().getEvents().getRegistry().getCount(), eventCount);
}

TEST(ReplicationPlugin, AcceptsTypesTheGameRegisteredIdentically) {
    App app;
    static_cast<void>(app.getWorld().getComponents().registerType<Owner>(
        {.presence = Presence::kBoth, .replication = ReplicationMode::kEveryChange}));
    static_cast<void>(app.getWorld().getEvents().registerType<ClientConnected>({.presence = Presence::kServerOnly}));

    EXPECT_NO_THROW(app.addPlugin(ReplicationPlugin{}));
    EXPECT_EQ(findComponent(app, "net.Owner")->type.replication, ReplicationMode::kEveryChange);
}

TEST(ReplicationPlugin, RefusesATypeTheGameRegisteredWithOtherOptions) {
    App app;
    static_cast<void>(app.getWorld().getComponents().registerType<Owner>());  // not replicated

    EXPECT_THROW(app.addPlugin(ReplicationPlugin{}), rtype::ecs::exceptions::TypeRegistrationException);
}

TEST(ReplicationPlugin, GivesEachComponentItsPresenceAndReplication) {
    App app;
    app.addPlugin(ReplicationPlugin{});

    const ComponentInfo* networkId = findComponent(app, "net.NetworkId");
    ASSERT_NE(networkId, nullptr);
    EXPECT_EQ(networkId->type.presence, Presence::kBoth);
    EXPECT_EQ(networkId->type.replication, ReplicationMode::kNone);

    const ComponentInfo* owner = findComponent(app, "net.Owner");
    ASSERT_NE(owner, nullptr);
    EXPECT_EQ(owner->type.presence, Presence::kBoth);
    EXPECT_EQ(owner->type.replication, ReplicationMode::kEveryChange);

    const ComponentInfo* input = findComponent(app, "net.PlayerInput");
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(input->type.presence, Presence::kBoth);
    EXPECT_EQ(input->type.replication, ReplicationMode::kNone);

    EXPECT_EQ(replicatedNames(app), std::vector<std::string_view>{"net.Owner"});
}

TEST(ReplicationPlugin, RegistersTheEventsOnTheServerOnly) {
    App app;
    app.addPlugin(ReplicationPlugin{});

    const EventInfo* connected = findEvent(app, "net.ClientConnected");
    ASSERT_NE(connected, nullptr);
    EXPECT_EQ(connected->type.presence, Presence::kServerOnly);

    const EventInfo* disconnected = findEvent(app, "net.ClientDisconnected");
    ASSERT_NE(disconnected, nullptr);
    EXPECT_EQ(disconnected->type.presence, Presence::kServerOnly);
}

TEST(ReplicationPlugin, LetsTheEventsBeSentAndRead) {
    App app;
    app.addPlugin(ReplicationPlugin{});
    auto& events = app.getWorld().getEvents();
    EventCursor connectedCursor;
    EventCursor disconnectedCursor;

    events.send(ClientConnected{.client = 7});
    events.send(ClientDisconnected{.client = 7, .reason = 2});

    std::vector<std::uint32_t> connected;
    for (const ClientConnected& event : events.read<ClientConnected>(connectedCursor)) {
        connected.push_back(event.client);
    }
    EXPECT_EQ(connected, std::vector<std::uint32_t>{7});

    std::vector<ClientDisconnected> disconnected;
    for (const ClientDisconnected& event : events.read<ClientDisconnected>(disconnectedCursor)) {
        disconnected.push_back(event);
    }
    ASSERT_EQ(disconnected.size(), 1U);
    EXPECT_EQ(disconnected[0].client, 7U);
    EXPECT_EQ(disconnected[0].reason, 2U);
}

TEST(ReplicationTypes, AreNamedUnderNet) {
    EXPECT_EQ(TypeTraits<NetworkId>::kName, "net.NetworkId");
    EXPECT_EQ(TypeTraits<Owner>::kName, "net.Owner");
    EXPECT_EQ(TypeTraits<PlayerInput>::kName, "net.PlayerInput");
    EXPECT_EQ(TypeTraits<ClientConnected>::kName, "net.ClientConnected");
    EXPECT_EQ(TypeTraits<ClientDisconnected>::kName, "net.ClientDisconnected");
}

TEST(ReplicationTypes, DefaultToZero) {
    EXPECT_EQ(NetworkId{}.value, 0U);
    EXPECT_EQ(Owner{}.client, 0U);

    const PlayerInput input{};
    EXPECT_EQ(input.buttons, 0U);
    EXPECT_EQ(input.previousButtons, 0U);
    EXPECT_TRUE(std::ranges::all_of(input.axes, [](float axis) { return axis == 0.0F; }));
    EXPECT_EQ(input.sequence, 0U);
}

TEST(ReplicationTypes, ReflectThePlayerInputAxesAsOneArrayField) {
    constexpr auto kFields = TypeTraits<PlayerInput>::kFields;
    ASSERT_EQ(kFields.size(), 4U);
    EXPECT_EQ(kFields[0].name, "buttons");
    EXPECT_EQ(kFields[0].type, FieldType::kU32);
    EXPECT_EQ(kFields[1].name, "previousButtons");
    EXPECT_EQ(kFields[1].type, FieldType::kU32);
    EXPECT_EQ(kFields[2].name, "axes");
    EXPECT_EQ(kFields[2].type, FieldType::kF32);
    EXPECT_EQ(kFields[2].count, kMaxInputAxes);
    EXPECT_EQ(kFields[3].name, "sequence");
    EXPECT_EQ(kFields[3].type, FieldType::kU32);
    EXPECT_EQ(kMaxInputAxes, 16U);
}

// The hard-coded hashes fail on purpose when a layout changes: the change then shows in review, and both sides of the
// network must be rebuilt together. Update them only on purpose.
TEST(ReplicationTypes, KeepTheirLayoutHashes) {
    EXPECT_EQ(TypeTraits<NetworkId>::kLayoutHash, 0x93825624FD42D660ULL);
    EXPECT_EQ(TypeTraits<Owner>::kLayoutHash, 0x0ACC4BF763CCD19FULL);
    EXPECT_EQ(TypeTraits<PlayerInput>::kLayoutHash, 0x3E5174BAA7A84CC5ULL);
    EXPECT_EQ(TypeTraits<ClientConnected>::kLayoutHash, 0x4599F89D69E61A4AULL);
    EXPECT_EQ(TypeTraits<ClientDisconnected>::kLayoutHash, 0x06C545FC9F117DA1ULL);
}

TEST(ReplicationTypes, RegisterWithTheirCompileTimeLayoutHash) {
    App app;
    app.addPlugin(ReplicationPlugin{});

    EXPECT_EQ(findComponent(app, "net.NetworkId")->type.layoutHash, TypeTraits<NetworkId>::kLayoutHash);
    EXPECT_EQ(findComponent(app, "net.Owner")->type.layoutHash, TypeTraits<Owner>::kLayoutHash);
    EXPECT_EQ(findComponent(app, "net.PlayerInput")->type.layoutHash, TypeTraits<PlayerInput>::kLayoutHash);
    EXPECT_EQ(findEvent(app, "net.ClientConnected")->type.layoutHash, TypeTraits<ClientConnected>::kLayoutHash);
    EXPECT_EQ(findEvent(app, "net.ClientDisconnected")->type.layoutHash, TypeTraits<ClientDisconnected>::kLayoutHash);
}
