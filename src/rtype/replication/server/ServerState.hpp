/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ServerState
*/

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/system/DynamicQueries.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/server/NetServer.hpp"
#include "rtype/network/server/RoomMessages.hpp"
#include "rtype/replication/Export.hpp"
#include "rtype/replication/ReplicationSettings.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/event/ClientConnected.hpp"
#include "rtype/replication/event/ClientDisconnected.hpp"
#include "rtype/replication/input/InputBuffer.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/manifest/Manifest.hpp"

namespace rtype::ecs {
class World;
}  // namespace rtype::ecs

namespace rtype::replication::server {
/// @brief What a network event asks the game to be told: a client joined, or left for good.
using ServerNotification = std::variant<event::ClientConnected, event::ClientDisconnected>;

/// @brief What the server's systems share: the transport, the clients, and the tables that tell which entity has
/// which NetworkId.
///
/// @details The protocol logic is here and not in the systems, which only pass it the events, the queries and the
/// time: every method but start(), receive() and sendSnapshots() needs no socket, so they are tested without an App. It
/// is an ECS resource, named "net.ServerState". It keeps a pointer to the world it was made for, which must outlive it.
class RTYPE_REPLICATION_API ServerState {
  public:
    /// @brief Gives the time of the transport.
    using TimeSource = std::function<network::TimePoint()>;

    /// @param network The transport settings.
    /// @param settings The replication settings. Must be valid when start() is called.
    /// @param world The world the server replicates; it must outlive the state. The components the snapshot reads are
    /// the ones registered now.
    ServerState(const network::NetworkSettings& network, const ReplicationSettings& settings, const ecs::World& world);
    ~ServerState();
    ServerState(const ServerState& other) = delete;
    ServerState& operator=(const ServerState& other) = delete;
    ServerState(ServerState&& other) noexcept = default;
    ServerState& operator=(ServerState&& other) noexcept = default;

    /// @return The server tick of the last snapshot built, 0 before any (also the reference of isSnapshotDue()).
    [[nodiscard]] std::uint32_t serverTick() const noexcept;

    /// @return The number of connected clients.
    [[nodiscard]] std::size_t clientCount() const noexcept;

    /// @return The number of entities with a NetworkId.
    [[nodiscard]] std::size_t entityCount() const noexcept;

    /// @return Whether start() succeeded.
    [[nodiscard]] bool isStarted() const noexcept;

    /// @return The transport, to read its statistics. Only after start().
    [[nodiscard]] network::server::NetServer& netServer() noexcept;

    /// @return The manifest sent to the clients. Only after start().
    [[nodiscard]] const manifest::Manifest& manifest() const noexcept;

    /// @return How many inputs were dropped because they could not be decoded.
    [[nodiscard]] std::size_t droppedInputCount() const noexcept;

    /// @return How many messages were ignored: of another type, or from a client that is not connected.
    [[nodiscard]] std::size_t ignoredMessageCount() const noexcept;

    /// @return How many snapshots did not list every entity because they did not fit in a message.
    [[nodiscard]] std::size_t incompleteSnapshotCount() const noexcept;

    /// @return Whether the client is connected.
    [[nodiscard]] bool hasClient(network::ClientId client) const noexcept;

    /// @return The tick of the last snapshot the client says it applied, 0 if none or if the client is unknown.
    [[nodiscard]] std::uint32_t lastAppliedSnapshot(network::ClientId client) const noexcept;

    /// @return The sequence of the last input of the client the server applied, 0 if none or if it is unknown.
    [[nodiscard]] std::uint32_t lastProcessedInput(network::ClientId client) const noexcept;

    /// @return The NetworkId of the entity, or nothing if it is not tracked.
    [[nodiscard]] std::optional<std::uint32_t> findNetworkId(ecs::Entity entity) const noexcept;

    /// @return The entity with this NetworkId, or nothing if none is tracked.
    [[nodiscard]] std::optional<ecs::Entity> findEntity(std::uint32_t networkId) const noexcept;

    /// @brief Replaces the source of the transport time (the default is network::Clock::now()), so that a test drives
    /// it with the clock of the App.
    void setTimeSource(TimeSource source);

    /// @brief Checks the settings and the replicated components, freezes the input actions and builds the manifest:
    /// what start() does before it needs a socket.
    /// @param inputs The game's input actions; frozen.
    /// @throws exceptions::ReplicationException When the snapshot rate does not divide the tick rate, or a replicated
    /// component was registered after the state was made.
    /// @throws exceptions::ManifestException When the manifest cannot be built.
    void prepare(input::InputLayout& inputs);

    /// @brief Prepares, opens the socket and gives the manifest to the clients that will join.
    /// @param inputs The game's input actions; frozen.
    /// @throws exceptions::ReplicationException See prepare().
    /// @throws exceptions::ManifestException See prepare().
    /// @throws network::exceptions::SocketException When the port cannot be bound.
    void start(input::InputLayout& inputs);

    /// @brief Receives what arrived, and handles every event.
    /// @param inputs The game's input actions.
    /// @return What the game has to be told, in order.
    [[nodiscard]] std::vector<ServerNotification> receive(const input::InputLayout& inputs);

    /// @brief Handles one event of the transport: a client joins, leaves, or sends an input.
    /// @param serverEvent The event.
    /// @param inputs The game's input actions, to decode inputs.
    /// @return What the game has to be told, if anything.
    [[nodiscard]] std::optional<ServerNotification> handle(const network::server::ServerEvent& serverEvent,
                                                           const input::InputLayout& inputs);

    /// @brief Writes the next input of a client in the PlayerInput of its entity: the neutral input when the client is
    /// not connected.
    /// @param client The owner of the entity.
    /// @param input The entity's PlayerInput.
    void applyInput(network::ClientId client, component::PlayerInput& input);

    /// @brief Gives an entity the next NetworkId, if it has none.
    /// @param entity An alive entity that has a replicated component.
    /// @return The new NetworkId (from 1, increasing, never reused), or nothing if the entity is tracked already.
    [[nodiscard]] std::optional<std::uint32_t> track(ecs::Entity entity);

    /// @param tick The current server tick (ecs::Time::tick).
    /// @return Whether a snapshot is due: at least `tickRate / snapshotRate` ticks passed since the last one.
    [[nodiscard]] bool isSnapshotDue(std::uint32_t tick) const noexcept;

    /// @brief Forgets the entities that are not alive, then writes one snapshot per connected client.
    /// @param tick The server tick the snapshots carry.
    /// @param replicated The queries of the replicated components, one per component in the order of
    /// query::listReplicatedComponents().
    /// @return One command per connected client, by client id; empty when nobody is connected.
    [[nodiscard]] std::vector<network::server::SendMessage> buildSnapshots(std::uint32_t tick,
                                                                           const ecs::DynamicQueries& replicated);

    /// @brief Builds the snapshots, hands them to the transport and sends them now.
    void sendSnapshots(std::uint32_t tick, const ecs::DynamicQueries& replicated);

  private:
    /// @brief What the server keeps of a connected client.
    struct ClientRecord {
        input::InputBuffer inputs;             ///< Its inputs, waiting for their tick.
        std::uint32_t lastAppliedSnapshot{0};  ///< The tick of the last snapshot it says it applied.
    };

    /// @brief Counts and reports a snapshot that did not list every entity.
    void reportIncomplete(std::size_t sent, std::size_t total);

    /// @brief Forgets the tracked entities that are no longer alive.
    void forgetDeadEntities();

    network::NetworkSettings _network;                             ///< The transport settings.
    ReplicationSettings _settings;                                 ///< The replication settings.
    const ecs::World* _world;                                      ///< The world replicated.
    std::vector<ecs::ComponentId> _replicated;                     ///< The components the queries were built from.
    TimeSource _time;                                              ///< The transport time.
    std::unique_ptr<network::server::NetServer> _server;           ///< Created by start() (a NetServer cannot move).
    manifest::Manifest _manifest;                                  ///< What the clients are told to expect.
    std::unordered_map<network::ClientId, ClientRecord> _clients;  ///< The connected clients.
    std::map<std::uint32_t, ecs::Entity> _entities;                ///< By NetworkId, in id order for snapshots.
    std::unordered_map<ecs::Entity, std::uint32_t> _ids;           ///< The NetworkId of each tracked entity.
    std::uint32_t _nextNetworkId{1};                               ///< The id the next tracked entity gets.
    std::uint32_t _serverTick{0};                                  ///< The tick of the last snapshot built.
    std::uint32_t _snapshotInterval{1};                            ///< Ticks between two snapshots, set by start().
    std::size_t _droppedInputs{0};                                 ///< Inputs that could not be decoded.
    std::size_t _ignoredMessages{0};                               ///< Messages that were not inputs of a client.
    std::size_t _incompleteSnapshots{0};                           ///< Snapshots that did not list every entity.
    std::optional<network::TimePoint> _lastWarning;                ///< When the last warning was written.
};

RTYPE_ECS_RESOURCE(ServerState, "net.ServerState");
}  // namespace rtype::replication::server
