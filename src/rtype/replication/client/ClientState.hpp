/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ClientState
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/client/ClientEvent.hpp"
#include "rtype/network/client/NetClient.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/replication/Export.hpp"
#include "rtype/replication/ReplicationSettings.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/input/InputMessage.hpp"
#include "rtype/replication/manifest/Manifest.hpp"
#include "rtype/replication/manifest/ManifestReceiver.hpp"
#include "rtype/replication/snapshot/Snapshot.hpp"

namespace rtype::ecs {
class Commands;
class World;
}  // namespace rtype::ecs

namespace rtype::replication::client {
/// @brief Where the check of the server's manifest stands.
enum class ManifestStatus : std::uint8_t {
    kWaiting,   ///< The manifest is not complete yet (or the client is not connected).
    kAccepted,  ///< The server and the client agree: snapshots are applied and inputs are sent.
    kMismatch,  ///< They differ: mismatch() tells how, and the client disconnects.
};

/// @brief Where the connection to the server stands.
enum class ConnectionStatus : std::uint8_t {
    kIdle,          ///< connect() was not called.
    kConnecting,    ///< The handshake is under way.
    kConnected,     ///< The server accepted the connection.
    kDisconnected,  ///< An accepted connection ended: see disconnectReason().
    kFailed,        ///< The connection could not be established: see rejectReason().
};

/// @brief What the client's systems share, and what the game reads: the transport, the manifest, the
/// `NetworkId → Entity` map and the inputs sent.
///
/// @details The protocol logic is here and not in the systems, which only pass it the layout and the command buffer:
/// only connect(), receive() and sendInput() need a socket, so handle() and applySnapshot() are tested without an App.
/// Nothing that comes from the network throws: a malformed message is dropped and counted. The writes to the world go
/// through the Commands, applied after the phase. It is an ECS resource, named "net.ClientState", and keeps a pointer
/// to the world it was made for, which must outlive it.
class RTYPE_REPLICATION_API ClientState {
  public:
    /// @brief Gives the time of the transport.
    using TimeSource = std::function<network::TimePoint()>;

    /// @param server The address of the server.
    /// @param settings The transport settings.
    /// @param world The client's world, once every plugin registered its types; it must outlive the state.
    ClientState(const network::io::Address& server, const network::NetworkSettings& settings, const ecs::World& world);
    ~ClientState();
    ClientState(const ClientState& other) = delete;
    ClientState& operator=(const ClientState& other) = delete;
    ClientState(ClientState&& other) noexcept = default;
    ClientState& operator=(ClientState&& other) noexcept = default;

    /// @return Where the check of the manifest stands.
    [[nodiscard]] ManifestStatus manifestStatus() const noexcept;

    /// @return What differs from the server, for the player; empty unless the status is kMismatch.
    [[nodiscard]] std::string_view mismatch() const noexcept;

    /// @return Where the connection stands.
    [[nodiscard]] ConnectionStatus connectionStatus() const noexcept;

    /// @return Why the accepted connection ended, if it did.
    [[nodiscard]] std::optional<network::DisconnectReason> disconnectReason() const noexcept;

    /// @return Why the server refused the connection, if it did (nothing when it did not answer either).
    [[nodiscard]] std::optional<network::RejectReason> rejectReason() const noexcept;

    /// @return The id the server gave to this client, once connected.
    [[nodiscard]] std::optional<network::ClientId> clientId() const noexcept;

    /// @return The settings the server sent at the end of the manifest (the defaults before).
    [[nodiscard]] const ReplicationSettings& serverSettings() const noexcept;

    /// @return The tick of the last snapshot applied, 0 before any.
    [[nodiscard]] std::uint32_t lastAppliedTick() const noexcept;

    /// @return The last input of this client the server had applied when it wrote the last snapshot applied, 0 if
    /// none.
    [[nodiscard]] std::uint32_t lastProcessedInput() const noexcept;

    /// @return How many messages were dropped because they could not be decoded or were not expected.
    [[nodiscard]] std::size_t droppedCount() const noexcept;

    /// @return How many snapshots were dropped because a newer one was applied already, or the manifest was not
    /// accepted yet.
    [[nodiscard]] std::size_t staleCount() const noexcept;

    /// @return The transport. Only after connect().
    [[nodiscard]] network::client::NetClient& netClient() noexcept;

    /// @brief Stores the input of the game for the frame; ignored until the manifest is accepted.
    /// @param input The input the game read from the player. Its buttons and axes are sent.
    void setInput(const component::PlayerInput& input);

    /// @param networkId The NetworkId of an entity of the server.
    /// @return The local entity, or the null entity if none is known.
    [[nodiscard]] ecs::Entity find(std::uint32_t networkId) const noexcept;

    /// @return The entity whose Owner is the id of this client, if there is one.
    [[nodiscard]] std::optional<ecs::Entity> localPlayer() const noexcept;

    /// @brief Replaces the source of the transport time (the default is network::Clock::now()), so that a test drives
    /// it with the clock of the App.
    void setTimeSource(TimeSource source);

    /// @brief Opens the socket and starts the handshake.
    /// @throws network::exceptions::SocketException When the socket cannot be opened.
    void connect();

    /// @brief Receives what arrived, and handles every event.
    /// @param inputs The client's input actions.
    /// @param commands Where the changes to the world are recorded.
    void receive(input::InputLayout& inputs, ecs::Commands& commands);

    /// @brief Handles one event of the transport: the connection, a message of the manifest, a snapshot.
    /// @param clientEvent The event.
    /// @param inputs The client's input actions; frozen once the manifest is checked.
    /// @param commands Where the changes to the world are recorded.
    void handle(const network::client::ClientEvent& clientEvent, input::InputLayout& inputs, ecs::Commands& commands);

    /// @brief Applies a snapshot, all or nothing, if it is newer than the last one applied.
    /// @param message The bytes of the message, starting with the message type.
    /// @param commands Where the changes to the world are recorded.
    /// @return Whether the snapshot was applied; a dropped one leaves the world and the state as they were.
    bool applySnapshot(std::span<const std::byte> message, ecs::Commands& commands);

    /// @brief Numbers the stored input and writes the kInput message: it and the two before, oldest first, and the
    /// tick of the last snapshot applied. Each call uses the next sequence (from 1).
    /// @param inputs The client's input actions.
    /// @return The bytes, or nothing until the manifest is accepted.
    [[nodiscard]] std::optional<network::Bytes> nextInputMessage(const input::InputLayout& inputs);

    /// @brief Sends nextInputMessage() unreliably and flushes. Does nothing when there is none or when not connected.
    /// @param inputs The client's input actions.
    void sendInput(const input::InputLayout& inputs);

  private:
    /// @brief Checks the manifest once it is complete; accepts it or disconnects.
    void checkManifest(input::InputLayout& inputs);

    /// @brief Tells whether a mapped entity exists, or is reserved by a command not flushed yet.
    [[nodiscard]] bool isLive(ecs::Entity entity) const;

    /// @brief Decodes every value of a snapshot without touching the world: whether it can be applied.
    [[nodiscard]] bool validate(const snapshot::Snapshot& decoded, const manifest::Manifest& manifest) const;

    /// @brief Records the spawn of every entity of the snapshot the client does not have.
    void spawnEntities(const snapshot::Snapshot& decoded, ecs::Commands& commands);

    /// @brief Records the values of the components of an entity.
    void writeValues(std::uint32_t networkId, const std::vector<snapshot::ComponentValue>& values,
                     const manifest::Manifest& manifest, ecs::Commands& commands) const;

    /// @brief Records the removal of the components the snapshot lists.
    void removeComponents(const snapshot::Snapshot& decoded, const manifest::Manifest& manifest,
                          ecs::Commands& commands) const;

    /// @brief Records the despawn of the entities the snapshot lists, and forgets them.
    void despawnEntities(const snapshot::Snapshot& decoded, ecs::Commands& commands);

    /// @brief Of a complete snapshot: records the despawn of the entities it does not list and the removal of their
    /// components it does not list.
    void removeUnlisted(const snapshot::Snapshot& decoded, const manifest::Manifest& manifest, ecs::Commands& commands);

    network::io::Address _server;                                ///< The server's address.
    network::NetworkSettings _settings;                          ///< The transport settings.
    const ecs::World* _world;                                    ///< The world replicated into.
    TimeSource _time;                                            ///< The transport time.
    std::unique_ptr<network::client::NetClient> _client;         ///< Created by connect().
    ConnectionStatus _connection{ConnectionStatus::kIdle};       ///< Where the connection stands.
    std::optional<network::ClientId> _clientId;                  ///< Given by the server.
    std::optional<network::DisconnectReason> _disconnectReason;  ///< Why the connection ended.
    std::optional<network::RejectReason> _rejectReason;          ///< Why the connection was refused.
    manifest::ManifestReceiver _receiver;                        ///< Collects the manifest.
    std::optional<manifest::Manifest> _manifest;                 ///< Set once accepted.
    ManifestStatus _manifestStatus{ManifestStatus::kWaiting};    ///< The result of the check.
    std::string _mismatch;                                       ///< The first difference, if any.
    std::vector<std::size_t> _componentSizes;                    ///< The wire size of each component by short id.
    std::unordered_map<std::uint32_t, ecs::Entity> _entities;    ///< NetworkId → local entity.
    std::unordered_set<ecs::Entity> _reserved;                   ///< Entities spawned by commands not flushed yet.
    std::uint32_t _lastAppliedTick{0};                           ///< The tick of the last snapshot applied.
    std::uint32_t _lastProcessedInput{0};                        ///< From the last snapshot applied.
    std::uint32_t _nextInputSequence{1};                         ///< The sequence of the next input sent.
    std::optional<component::PlayerInput> _input;                ///< The game's input for the frame.
    std::deque<input::InputSample> _recentInputs;                ///< The last three, oldest first.
    std::size_t _dropped{0};                                     ///< Messages that could not be used.
    std::size_t _stale{0};                                       ///< Snapshots that were not applied.
};

RTYPE_ECS_RESOURCE(ClientState, "net.ClientState");
}  // namespace rtype::replication::client
