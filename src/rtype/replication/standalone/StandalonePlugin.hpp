/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** StandalonePlugin
*/

#pragma once

#include <string_view>

#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/replication/Export.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::replication::standalone {
/// @brief The replication of a standalone world: a server with one local client, client 1, and no socket, so that a
/// networked game written once runs unchanged.
///
/// @details Adds the ReplicationPlugin (so every component, event, call and form exists), the LocalSession resource
/// and these systems:
///
/// | Phase       | System             | What it does                                                           |
/// |-------------|--------------------|------------------------------------------------------------------------|
/// | PreUpdate   | `net.receive`      | raises `net.ClientConnected` for client 1 once; delivers the packets   |
///
/// `rtype.replication.localClient` answers `{connected = true, client = 1}`, and `rtype.net.sendToServer` and
/// `rtype.net.sendToClients` loop the packet back into the world, one frame later, with the same checks as the
/// network. A packet to the server has `sender` 1, one to the clients has `sender` 0: scripts tell the side they are
/// for from it (`engine.net.received`).
class RTYPE_REPLICATION_API StandalonePlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.replication.standalone";  ///< The plugin's name.
    static constexpr std::string_view kReceiveSystem = "net.receive";          ///< PreUpdate.

    StandalonePlugin() = default;
    ~StandalonePlugin() override;
    StandalonePlugin(const StandalonePlugin& other) = default;
    StandalonePlugin& operator=(const StandalonePlugin& other) = default;
    StandalonePlugin(StandalonePlugin&& other) noexcept = default;
    StandalonePlugin& operator=(StandalonePlugin&& other) noexcept = default;

    /// @return "rtype.replication.standalone".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Inserts the LocalSession, adds the ReplicationPlugin, adds the call and the systems.
    void build(ecs::App& app) override;
};
}  // namespace rtype::replication::standalone
