/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ServerPlugin
*/

#include "rtype/replication/server/ServerPlugin.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryMatch.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/DynamicQueries.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/time/Time.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/replication/ReplicationPlugin.hpp"
#include "rtype/replication/ReplicationSettings.hpp"
#include "rtype/replication/component/NetworkId.hpp"
#include "rtype/replication/component/Owner.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/event/ClientConnected.hpp"
#include "rtype/replication/event/ClientDisconnected.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/query/ReplicatedQueries.hpp"
#include "rtype/replication/server/ServerState.hpp"

namespace rtype::replication::server {
namespace {
/// @brief Startup: opens the server.
void start(ecs::ResMut<ServerState> state, ecs::ResMut<input::InputLayout> inputs) { state->start(*inputs); }

/// @brief PreUpdate: receives, and tells the game who joined and left.
void receive(ecs::ResMut<ServerState> state, ecs::Res<input::InputLayout> inputs,
             ecs::EventWriter<event::ClientConnected> connected, ecs::EventWriter<event::ClientDisconnected> left) {
    for (const ServerNotification& notification : state->receive(inputs.get())) {
        if (const auto* joined = std::get_if<event::ClientConnected>(&notification)) {
            connected.send(*joined);
        } else {
            left.send(std::get<event::ClientDisconnected>(notification));
        }
    }
}

/// @brief FixedUpdate: writes the next input of each client in the entity it owns.
void applyInputs(ecs::ResMut<ServerState> state, ecs::Query<component::PlayerInput, const component::Owner> players) {
    for (auto [entity, input, owner] : players) {
        state->applyInput(owner.client, *input);
    }
}

/// @brief PostUpdate: gives a NetworkId to the entities that have a replicated component and none.
void assignIds(ecs::ResMut<ServerState> state, ecs::Commands& commands, ecs::DynamicQueries untracked) {
    for (std::size_t query = 0; query < untracked.getCount(); ++query) {
        for (const ecs::QueryMatch& match : untracked.iter(query)) {
            if (const auto networkId = state->track(match.getEntity())) {
                commands.add(match.getEntity(), component::NetworkId{.value = *networkId});
            }
        }
    }
}

/// @brief PostUpdate: sends a snapshot to every client, when one is due.
void snapshot(ecs::ResMut<ServerState> state, ecs::Res<ecs::Time> time, ecs::DynamicQueries replicated) {
    const std::uint32_t tick = time->tick;
    if (state->isSnapshotDue(tick)) {
        state->sendSnapshots(tick, replicated);
    }
}
}  // namespace

ServerPlugin::ServerPlugin(const network::NetworkSettings& network, const ReplicationSettings& settings)
    : _network{network}, _settings{settings} {}

ServerPlugin::~ServerPlugin() = default;

std::string_view ServerPlugin::getName() const noexcept { return kName; }

void ServerPlugin::build(ecs::App& app) {
    app.addPlugin(ReplicationPlugin{});
    app.insertResource(ServerState{_network, _settings, app.getWorld()});

    app.addSystem(ecs::Phase::kStartup, std::string{kStartSystem}, &start);
    app.addSystem(ecs::Phase::kPreUpdate, std::string{kReceiveSystem}, &receive);
    app.addSystem(ecs::Phase::kFixedUpdate, std::string{kApplyInputsSystem}, &applyInputs);
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kAssignIdsSystem}, &assignIds,
                  query::makeUntrackedQueries(app.getWorld()));
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kSnapshotSystem}, &snapshot,
                  query::makeReplicatedReads(app.getWorld()))
        .after(std::string{kAssignIdsSystem});
}
}  // namespace rtype::replication::server
