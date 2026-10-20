/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ServerPlugin
*/

#pragma once

#include <string_view>

#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/replication/Export.hpp"
#include "rtype/replication/ReplicationSettings.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::replication::server {
/// @brief The server's replication: opens the NetServer, hands out NetworkIds, applies inputs and sends snapshots.
///
/// @details An ordinary plugin: build() only registers (the ReplicationPlugin, the ServerState resource and the
/// systems, all on the server side), and the work is done by systems on the phases of the frame:
///
/// | Phase       | System           | What it does                                                           |
/// |-------------|------------------|------------------------------------------------------------------------|
/// | Startup     | `net.start`      | binds the socket, builds the manifest and gives it to the clients      |
/// | PreUpdate   | `net.receive`    | polls the transport; sends ClientConnected / ClientDisconnected; keeps inputs |
/// | FixedUpdate | `net.applyInputs`| writes each owned entity's PlayerInput from its client's next input    |
/// | PostUpdate  | `net.assignIds`  | gives a NetworkId to every new entity with a replicated component      |
/// | PostUpdate  | `net.snapshot`   | sends every client a complete snapshot, every `tickRate / snapshotRate` ticks |
///
/// Add it after the plugins that register replicated components: the snapshot reads the ones registered when it is
/// built, and net.start refuses to run if one was registered later. Game systems that read inputs run
/// `.after(ServerPlugin::kApplyInputsSystem)`; a game system of PostUpdate that writes a replicated component runs
/// `.before(ServerPlugin::kSnapshotSystem)` (or after it), as the snapshot reads them all.
/// @code
/// app.addPlugin(GamePlugin{});  // registers Position, Health...
/// app.addPlugin(rtype::replication::server::ServerPlugin{rtype::network::NetworkSettings{}, {}});
/// app.addSystem(rtype::ecs::Phase::kFixedUpdate, "game.ships", &moveShips)
///     .after(std::string{rtype::replication::server::ServerPlugin::kApplyInputsSystem});
/// @endcode
class RTYPE_REPLICATION_API ServerPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.replication.server";      ///< The plugin's name.
    static constexpr std::string_view kStartSystem = "net.start";              ///< Startup: opens the server.
    static constexpr std::string_view kReceiveSystem = "net.receive";          ///< PreUpdate: receives.
    static constexpr std::string_view kApplyInputsSystem = "net.applyInputs";  ///< FixedUpdate: game systems run after.
    static constexpr std::string_view kAssignIdsSystem = "net.assignIds";      ///< PostUpdate: gives NetworkIds.
    static constexpr std::string_view kSnapshotSystem = "net.snapshot";        ///< PostUpdate: sends snapshots.

    /// @param network The transport settings (port, limits, delays).
    /// @param settings The tick rate and the snapshot rate, which divides it.
    ServerPlugin(const network::NetworkSettings& network, const ReplicationSettings& settings);
    ~ServerPlugin() override;
    ServerPlugin(const ServerPlugin& other) = default;
    ServerPlugin& operator=(const ServerPlugin& other) = default;
    ServerPlugin(ServerPlugin&& other) noexcept = default;
    ServerPlugin& operator=(ServerPlugin&& other) noexcept = default;

    /// @return "rtype.replication.server".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Adds the ReplicationPlugin, inserts the ServerState and adds the systems.
    void build(ecs::App& app) override;

  private:
    network::NetworkSettings _network;  ///< The transport settings.
    ReplicationSettings _settings;      ///< The replication settings.
};
}  // namespace rtype::replication::server
