/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ReplicationPlugin
*/

#include "rtype/replication/ReplicationPlugin.hpp"

#include <string_view>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/replication/client/ClientCalls.hpp"
#include "rtype/replication/component/NetworkId.hpp"
#include "rtype/replication/component/Owner.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/event/ClientConnected.hpp"
#include "rtype/replication/event/ClientDisconnected.hpp"
#include "rtype/replication/input/InputCalls.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/packet/PacketCalls.hpp"
#include "rtype/replication/packet/PacketSchema.hpp"
#include "rtype/replication/standalone/LocalSession.hpp"

namespace rtype::replication {
ReplicationPlugin::~ReplicationPlugin() = default;  // defined here: the class's vtable lives in the library

std::string_view ReplicationPlugin::getName() const noexcept { return kName; }

void ReplicationPlugin::build(ecs::App& app) {
    ecs::Components& components = app.getWorld().getComponents();
    static_cast<void>(components.registerType<component::NetworkId>(
        {.presence = ecs::Presence::kBoth, .replication = ecs::ReplicationMode::kNone}));
    static_cast<void>(components.registerType<component::Owner>(
        {.presence = ecs::Presence::kBoth, .replication = ecs::ReplicationMode::kEveryChange}));
    static_cast<void>(components.registerType<ecs::Parent>(ecs::kParentOptions));
    static_cast<void>(components.registerType<component::PlayerInput>(
        {.presence = ecs::Presence::kBoth, .replication = ecs::ReplicationMode::kNone}));

    ecs::Events& events = app.getWorld().getEvents();
    static_cast<void>(events.registerType<event::ClientConnected>({.presence = ecs::Presence::kServerOnly}));
    static_cast<void>(events.registerType<event::ClientDisconnected>({.presence = ecs::Presence::kServerOnly}));

    static_cast<void>(events.registerType<packet::PacketReceived>({.presence = ecs::Presence::kBoth}));

    app.insertResourceIfAbsent<input::InputLayout>();
    app.insertResourceIfAbsent<packet::PacketRegistry>();
    packet::addPacketCalls(app);
    input::addInputCalls(app);
    // A standalone session has no ClientState: the StandalonePlugin answers localClient.
    const ecs::RunMode mode = app.getSchedules().getRunMode();
    if (!app.getWorld().getResources().has<standalone::LocalSession>() && ecs::runsIn(ecs::Side::kClient, mode)) {
        client::addClientCalls(app);
    }
}
}  // namespace rtype::replication
