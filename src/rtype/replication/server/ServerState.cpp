/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ServerState
*/

#include "rtype/replication/server/ServerState.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/QueryMatch.hpp"
#include "rtype/ecs/system/DynamicQueries.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/world/World.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/bytes/ByteWriter.hpp"
#include "rtype/network/protocol/Constants.hpp"
#include "rtype/network/server/NetServer.hpp"
#include "rtype/network/server/RoomMessages.hpp"
#include "rtype/replication/MessageType.hpp"
#include "rtype/replication/ReplicationSettings.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/event/ClientConnected.hpp"
#include "rtype/replication/event/ClientDisconnected.hpp"
#include "rtype/replication/exceptions/ReplicationExceptions.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/input/InputMessage.hpp"
#include "rtype/replication/query/ReplicatedQueries.hpp"
#include "rtype/replication/serialization/EntityMapper.hpp"
#include "rtype/replication/serialization/FieldSerializer.hpp"
#include "rtype/replication/snapshot/Snapshot.hpp"

namespace rtype::replication::server {
namespace {
/// @brief The shortest delay between two warnings about snapshots that do not fit.
constexpr std::chrono::seconds kWarningInterval{5};

/// @brief Gives the NetworkId of the entities the server tracks.
class TrackingMapper final : public serialization::EntityMapper {
  public:
    /// @param ids The NetworkId of each tracked entity; it must outlive the mapper.
    explicit TrackingMapper(const std::unordered_map<ecs::Entity, std::uint32_t>& ids) noexcept : _ids{&ids} {}
    ~TrackingMapper() override = default;
    TrackingMapper(const TrackingMapper& other) = delete;
    TrackingMapper& operator=(const TrackingMapper& other) = delete;
    TrackingMapper(TrackingMapper&& other) = delete;
    TrackingMapper& operator=(TrackingMapper&& other) = delete;

    [[nodiscard]] std::uint32_t toNetwork(const ecs::Entity entity) const noexcept override {
        const auto found = _ids->find(entity);
        return found == _ids->end() ? 0 : found->second;
    }

    /// @return The null entity: the server never reads entities from the network.
    [[nodiscard]] ecs::Entity toLocal(const std::uint32_t /*networkId*/) const noexcept override { return {}; }

  private:
    const std::unordered_map<ecs::Entity, std::uint32_t>* _ids;  ///< The NetworkId of each tracked entity.
};

/// @brief Writes the value of a component, if the entity's bytes are a whole value.
[[nodiscard]] std::optional<network::Bytes> serializeValue(const ecs::TypeInfo& type, const std::size_t wireSize,
                                                           const std::span<const std::byte> value,
                                                           const serialization::EntityMapper& mapper) {
    if (value.size() < type.size) {
        return std::nullopt;
    }
    network::Bytes bytes(wireSize);
    network::bytes::ByteWriter writer{bytes};
    serialization::writeFields(writer, type, value, mapper);
    if (!writer.isValid()) {
        return std::nullopt;
    }
    return bytes;
}
}  // namespace

ServerState::ServerState(const network::NetworkSettings& network, const ReplicationSettings& settings,
                         const ecs::World& world)
    : _network{network},
      _settings{settings},
      _world{&world},
      _replicated{query::listReplicatedComponents(world)},
      _time{[] { return network::Clock::now(); }} {}

ServerState::~ServerState() = default;

std::uint32_t ServerState::serverTick() const noexcept { return _serverTick; }

std::size_t ServerState::clientCount() const noexcept { return _clients.size(); }

std::size_t ServerState::entityCount() const noexcept { return _entities.size(); }

bool ServerState::isStarted() const noexcept { return _server != nullptr; }

network::server::NetServer& ServerState::netServer() noexcept { return *_server; }

const manifest::Manifest& ServerState::manifest() const noexcept { return _manifest; }

std::size_t ServerState::droppedInputCount() const noexcept { return _droppedInputs; }

std::size_t ServerState::ignoredMessageCount() const noexcept { return _ignoredMessages; }

std::size_t ServerState::incompleteSnapshotCount() const noexcept { return _incompleteSnapshots; }

bool ServerState::hasClient(const network::ClientId client) const noexcept { return _clients.contains(client); }

std::uint32_t ServerState::lastAppliedSnapshot(const network::ClientId client) const noexcept {
    const auto found = _clients.find(client);
    return found == _clients.end() ? 0 : found->second.lastAppliedSnapshot;
}

std::uint32_t ServerState::lastProcessedInput(const network::ClientId client) const noexcept {
    const auto found = _clients.find(client);
    return found == _clients.end() ? 0 : found->second.inputs.lastProcessed();
}

std::optional<std::uint32_t> ServerState::findNetworkId(const ecs::Entity entity) const noexcept {
    const auto found = _ids.find(entity);
    if (found == _ids.end()) {
        return std::nullopt;
    }
    return found->second;
}

std::optional<ecs::Entity> ServerState::findEntity(const std::uint32_t networkId) const noexcept {
    const auto found = _entities.find(networkId);
    if (found == _entities.end()) {
        return std::nullopt;
    }
    return found->second;
}

void ServerState::setTimeSource(TimeSource source) { _time = std::move(source); }

void ServerState::prepare(input::InputLayout& inputs) {
    if (_settings.tickRate == 0 || _settings.snapshotRate == 0 || _settings.tickRate % _settings.snapshotRate != 0) {
        throw exceptions::ReplicationException{
            std::format("The snapshot rate ({} Hz) must be above 0 and divide the tick rate ({} Hz)",
                        _settings.snapshotRate, _settings.tickRate)};
    }
    const auto& registry = _world->getComponents().getRegistry();
    for (const ecs::ComponentId id : query::listReplicatedComponents(*_world)) {
        if (std::ranges::find(_replicated, id) == _replicated.end()) {
            const ecs::ComponentInfo* info = registry.findById(id);
            throw exceptions::ReplicationException{std::format(
                "The replicated component '{}' was registered after the server plugin: add the plugin that registers "
                "it before the server",
                info == nullptr ? "?" : info->type.name)};
        }
    }
    inputs.freeze();
    _manifest = manifest::Manifest::build(*_world, inputs);
    _snapshotInterval = _settings.tickRate / _settings.snapshotRate;
}

void ServerState::start(input::InputLayout& inputs) {
    prepare(inputs);
    _server = std::make_unique<network::server::NetServer>(_network);
    _server->setWelcomeMessages(_manifest.encode(_settings));
}

std::vector<ServerNotification> ServerState::receive(const input::InputLayout& inputs) {
    std::vector<ServerNotification> notifications;
    _server->poll(_time());
    for (const network::server::ServerEvent& serverEvent : _server->takeEvents()) {
        if (std::optional<ServerNotification> notification = handle(serverEvent, inputs)) {
            notifications.push_back(*notification);
        }
    }
    return notifications;
}

std::optional<ServerNotification> ServerState::handle(const network::server::ServerEvent& serverEvent,
                                                      const input::InputLayout& inputs) {
    if (const auto* joined = std::get_if<network::server::ClientJoined>(&serverEvent)) {
        _clients.insert_or_assign(joined->client, ClientRecord{});
        return event::ClientConnected{.client = joined->client};
    }
    if (const auto* left = std::get_if<network::server::ClientLeft>(&serverEvent)) {
        if (_clients.erase(left->client) == 0) {
            return std::nullopt;
        }
        return event::ClientDisconnected{.client = left->client, .reason = static_cast<std::uint8_t>(left->reason)};
    }
    const auto& message = std::get<network::server::ClientMessage>(serverEvent);
    const auto client = _clients.find(message.client);
    if (client == _clients.end() || message.payload.empty() ||
        message.payload.front() != static_cast<std::byte>(MessageType::kInput)) {
        ++_ignoredMessages;
        return std::nullopt;
    }
    const std::optional<input::InputMessage> decoded = input::decodeInput(message.payload, inputs);
    if (!decoded) {
        ++_droppedInputs;
        return std::nullopt;
    }
    for (const input::InputSample& sample : decoded->inputs) {
        client->second.inputs.push(sample);
    }
    client->second.lastAppliedSnapshot = std::max(client->second.lastAppliedSnapshot, decoded->lastAppliedSnapshot);
    return std::nullopt;
}

void ServerState::applyInput(const network::ClientId client, component::PlayerInput& input) {
    const auto found = _clients.find(client);
    input.previousButtons = input.buttons;
    if (found == _clients.end()) {
        input.buttons = 0;
        input.axes = {};
        input.sequence = 0;
        return;
    }
    const input::InputSample& sample = found->second.inputs.next();
    input.buttons = sample.buttons;
    input.axes = sample.axes;
    input.sequence = sample.sequence;
}

std::optional<std::uint32_t> ServerState::track(const ecs::Entity entity) {
    if (_ids.contains(entity)) {
        return std::nullopt;
    }
    const std::uint32_t networkId = _nextNetworkId++;
    _ids.emplace(entity, networkId);
    _entities.emplace(networkId, entity);
    return networkId;
}

bool ServerState::isSnapshotDue(const std::uint32_t tick) const noexcept {
    return tick - _serverTick >= _snapshotInterval;
}

void ServerState::forgetDeadEntities() {
    for (auto entry = _entities.begin(); entry != _entities.end();) {
        if (_world->isAlive(entry->second)) {
            ++entry;
            continue;
        }
        _ids.erase(entry->second);
        entry = _entities.erase(entry);
    }
}

void ServerState::reportIncomplete(const std::size_t sent, const std::size_t total) {
    ++_incompleteSnapshots;
    const network::TimePoint now = _time();
    if (_lastWarning && now - *_lastWarning < kWarningInterval) {
        return;
    }
    _lastWarning = now;
    std::clog << std::format("[replication] a snapshot lists {} of {} entities: it does not fit in {} bytes\n", sent,
                             total, network::protocol::kMaxMessageSize);
}

std::vector<network::server::SendMessage> ServerState::buildSnapshots(const std::uint32_t tick,
                                                                      const ecs::DynamicQueries& replicated) {
    _serverTick = tick;
    forgetDeadEntities();
    if (_clients.empty()) {
        return {};
    }

    const TrackingMapper mapper{_ids};
    const auto& registry = _world->getComponents().getRegistry();
    std::map<std::uint32_t, snapshot::SpawnRecord> records;
    for (std::size_t query = 0; query < std::min(replicated.getCount(), _replicated.size()); ++query) {
        const ecs::ComponentInfo* info = registry.findById(_replicated.at(query));
        const std::optional<std::uint16_t> shortId = _manifest.toShort(_replicated.at(query));
        if (info == nullptr || !shortId) {
            continue;
        }
        const std::size_t wireSize = serialization::computeWireSize(info->type);
        for (const ecs::QueryMatch& match : replicated.iter(query)) {
            const std::optional<std::uint32_t> networkId = findNetworkId(match.getEntity());
            std::optional<network::Bytes> bytes = serializeValue(info->type, wireSize, match.get(0), mapper);
            if (!networkId || !bytes) {
                continue;
            }
            snapshot::SpawnRecord& record = records.try_emplace(*networkId).first->second;
            record.networkId = *networkId;
            record.components.push_back({.shortId = *shortId, .bytes = std::move(*bytes)});
        }
    }

    snapshot::Snapshot body{.tick = tick};
    std::size_t size = snapshot::kEmptySize;
    for (auto& [networkId, record] : records) {
        std::ranges::sort(record.components, std::ranges::less{}, &snapshot::ComponentValue::shortId);
        const std::size_t recordSize = snapshot::computeWireSize(record);
        if (record.components.size() <= snapshot::kMaxRecordComponents &&
            size + recordSize <= network::protocol::kMaxMessageSize) {
            size += recordSize;
            body.spawns.push_back(std::move(record));
        }
    }
    if (body.spawns.size() == records.size()) {
        body.flags = snapshot::kComplete;
    } else {
        reportIncomplete(body.spawns.size(), records.size());
    }
    const std::optional<network::Bytes> encoded = snapshot::encode(body);
    if (!encoded) {
        return {};
    }

    std::vector<network::ClientId> clients;
    clients.reserve(_clients.size());
    for (const auto& [client, record] : _clients) {
        clients.push_back(client);
    }
    std::ranges::sort(clients);
    std::vector<network::server::SendMessage> commands;
    commands.reserve(clients.size());
    for (const network::ClientId client : clients) {
        network::Bytes payload = *encoded;
        snapshot::setLastProcessedInput(payload, _clients.at(client).inputs.lastProcessed());
        commands.push_back({.client = client, .channel = network::Channel::kUnreliable, .payload = std::move(payload)});
    }
    return commands;
}

void ServerState::sendSnapshots(const std::uint32_t tick, const ecs::DynamicQueries& replicated) {
    for (network::server::SendMessage& command : buildSnapshots(tick, replicated)) {
        _server->execute(std::move(command));
    }
    _server->flush(_time());
}
}  // namespace rtype::replication::server
