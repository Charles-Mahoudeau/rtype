/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ClientPlugin
*/

#pragma once

#include <string_view>

#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/replication/Export.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::replication::client {
/// @brief The client's replication: connects, checks the manifest, applies snapshots and sends the player's inputs.
///
/// @details An ordinary plugin: build() only registers (the ReplicationPlugin, the ClientState resource and the
/// systems, all on the client side), and the work is done by systems on the phases of the frame:
///
/// | Phase       | System          | What it does                                                              |
/// |-------------|-----------------|---------------------------------------------------------------------------|
/// | Startup     | `net.connect`   | opens the socket and starts connecting                                    |
/// | PreUpdate   | `net.receive`   | polls the transport; checks the manifest; applies the snapshots           |
/// | PostUpdate  | `net.sendInput` | numbers the input the game gave and sends it with the two before          |
///
/// The entities of a snapshot are created by commands, so they exist from the phase after PreUpdate. The game reads
/// the player's input and calls ClientState::setInput() in a system that runs
/// `.before(ClientPlugin::kSendInputSystem)`. Add the plugin after the ones that register replicated components.
/// @code
/// app.addPlugin(GamePlugin{});
/// app.addPlugin(rtype::replication::client::ClientPlugin{server, rtype::network::NetworkSettings{}});
/// app.addSystem(rtype::ecs::Phase::kPostUpdate, "game.input", &sendInput)
///     .before(std::string{rtype::replication::client::ClientPlugin::kSendInputSystem});
/// @endcode
class RTYPE_REPLICATION_API ClientPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.replication.client";  ///< The plugin's name.
    static constexpr std::string_view kConnectSystem = "net.connect";      ///< Startup: connects.
    static constexpr std::string_view kReceiveSystem = "net.receive";      ///< PreUpdate: receives.
    static constexpr std::string_view kSendInputSystem =
        "net.sendInput";  ///< PostUpdate: the game's input runs before.

    /// @param server The address of the server.
    /// @param settings The transport settings.
    ClientPlugin(const network::io::Address& server, const network::NetworkSettings& settings);
    ~ClientPlugin() override;
    ClientPlugin(const ClientPlugin& other) = default;
    ClientPlugin& operator=(const ClientPlugin& other) = default;
    ClientPlugin(ClientPlugin&& other) noexcept = default;
    ClientPlugin& operator=(ClientPlugin&& other) noexcept = default;

    /// @return "rtype.replication.client".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Adds the ReplicationPlugin, inserts the ClientState and adds the systems.
    void build(ecs::App& app) override;

  private:
    network::io::Address _server;        ///< The address of the server.
    network::NetworkSettings _settings;  ///< The transport settings.
};
}  // namespace rtype::replication::client
