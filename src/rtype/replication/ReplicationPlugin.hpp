/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ReplicationPlugin
*/

#pragma once

#include <string_view>

#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/replication/Export.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::replication {
/// @brief Registers the components and events through which a game takes part in replication.
///
/// @details Registers NetworkId, Owner and PlayerInput (components) and ClientConnected and ClientDisconnected
/// (server-only events), with the presence and replication mode of each. It only registers: it adds no system and
/// does no I/O. The server and client plugins add it from their build(), so the types are registered once on both
/// sides, as the manifest requires; a standalone game adds it alone.
/// @code
/// app.addPlugin(rtype::replication::ReplicationPlugin{});
/// @endcode
class RTYPE_REPLICATION_API ReplicationPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.replication";  ///< The plugin's name.

    ReplicationPlugin() = default;
    ~ReplicationPlugin() override;
    ReplicationPlugin(const ReplicationPlugin& other) = default;
    ReplicationPlugin& operator=(const ReplicationPlugin& other) = default;
    ReplicationPlugin(ReplicationPlugin&& other) noexcept = default;
    ReplicationPlugin& operator=(ReplicationPlugin&& other) noexcept = default;

    /// @return "rtype.replication".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Registers every replication component and event.
    /// @throws ecs::exceptions::TypeRegistrationException When the game registered one of these names with other
    /// options or another layout.
    void build(ecs::App& app) override;
};
}  // namespace rtype::replication
