/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ClientPlugin
*/

#include "rtype/replication/client/ClientPlugin.hpp"

#include <string>
#include <string_view>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/replication/ReplicationPlugin.hpp"
#include "rtype/replication/client/ClientState.hpp"
#include "rtype/replication/input/InputLayout.hpp"

namespace rtype::replication::client {
namespace {
/// @brief Startup: opens the socket and starts connecting.
void connect(ecs::ResMut<ClientState> state) { state->connect(); }

/// @brief PreUpdate: receives, checks the manifest and records the snapshots in the commands.
void receive(ecs::ResMut<ClientState> state, ecs::ResMut<input::InputLayout> inputs, ecs::Commands& commands) {
    state->receive(*inputs, commands);
}

/// @brief PostUpdate: sends the input of the frame.
void sendInput(ecs::ResMut<ClientState> state, ecs::Res<input::InputLayout> inputs) { state->sendInput(*inputs); }
}  // namespace

ClientPlugin::ClientPlugin(const network::io::Address& server, const network::NetworkSettings& settings)
    : _server{server}, _settings{settings} {}

ClientPlugin::~ClientPlugin() = default;

std::string_view ClientPlugin::getName() const noexcept { return kName; }

void ClientPlugin::build(ecs::App& app) {
    app.addPlugin(ReplicationPlugin{});
    app.insertResource(ClientState{_server, _settings, app.getWorld()});

    app.addSystem(ecs::Phase::kStartup, std::string{kConnectSystem}, &connect).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kPreUpdate, std::string{kReceiveSystem}, &receive).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kSendInputSystem}, &sendInput).setSide(ecs::Side::kClient);
}
}  // namespace rtype::replication::client
