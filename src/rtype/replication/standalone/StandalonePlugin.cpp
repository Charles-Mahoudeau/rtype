/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** StandalonePlugin
*/

#include "rtype/replication/standalone/StandalonePlugin.hpp"

#include <string>
#include <string_view>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/replication/ReplicationPlugin.hpp"
#include "rtype/replication/client/ClientCalls.hpp"
#include "rtype/replication/component/Owner.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/event/ClientConnected.hpp"
#include "rtype/replication/packet/PacketCalls.hpp"
#include "rtype/replication/packet/PacketCodec.hpp"
#include "rtype/replication/packet/PacketSchema.hpp"
#include "rtype/replication/standalone/LocalSession.hpp"

namespace rtype::replication::standalone {
namespace {
/// @brief `rtype.replication.localClient`: the local client is always connected.
client::LocalClient localClient() { return client::LocalClient{.connected = true, .client = LocalSession::kClient}; }

/// @brief PreUpdate: announces the local client once and delivers the packets sent since the last frame.
void receive(ecs::ResMut<LocalSession> session, ecs::Res<packet::PacketRegistry> packets,
             ecs::EventWriter<event::ClientConnected> connected, ecs::EventWriter<packet::PacketReceived> received) {
    if (session->takeConnect()) {
        connected.send(event::ClientConnected{.client = LocalSession::kClient});
    }
    for (const QueuedPacket& queued : session->takePackets()) {
        const auto decoded = packet::decodePacket(*packets, queued.direction, queued.message);
        if (!decoded) {
            continue;  // cannot happen for a packet the send call encoded; dropped like a bad network one
        }
        const packet::PacketSchema* schema = packets->find(decoded->name);
        if (schema == nullptr) {
            continue;
        }
        received.send(packet::PacketReceived{
            .name = decoded->name,
            .sender = queued.direction == packet::PacketDirection::kToServer ? LocalSession::kClient : 0,
            .values = packet::formatValues(*schema, decoded->values),
        });
    }
}

/// @brief FixedUpdate: writes the local input in the entity of client 1, as the server does with a client's.
void applyInputs(ecs::Res<LocalSession> session, ecs::Query<component::PlayerInput, const component::Owner> players) {
    for (auto [entity, input, owner] : players) {
        input->previousButtons = input->buttons;
        if (owner.client == LocalSession::kClient) {
            input->buttons = session->input().buttons;
            input->axes = session->input().axes;
            input->sequence = session->input().sequence;
        } else {
            input->buttons = 0;
            input->axes = {};
            input->sequence = 0;
        }
    }
}
}  // namespace

StandalonePlugin::~StandalonePlugin() = default;

std::string_view StandalonePlugin::getName() const noexcept { return kName; }

void StandalonePlugin::build(ecs::App& app) {
    // First: the ReplicationPlugin picks the local calls when it finds the session.
    app.insertResource(LocalSession{});
    app.addPlugin(ReplicationPlugin{});
    app.addCall(std::string{client::kLocalClientCall}, &localClient);
    app.addSystem(ecs::Phase::kPreUpdate, std::string{kReceiveSystem}, &receive);
    app.addSystem(ecs::Phase::kFixedUpdate, std::string{kApplyInputsSystem}, &applyInputs);
}
}  // namespace rtype::replication::standalone
