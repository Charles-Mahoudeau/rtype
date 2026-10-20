/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ClientState
*/

#include "rtype/replication/client/ClientState.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/world/World.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/bytes/ByteReader.hpp"
#include "rtype/network/client/ClientEvent.hpp"
#include "rtype/network/client/NetClient.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/replication/MessageType.hpp"
#include "rtype/replication/ReplicationSettings.hpp"
#include "rtype/replication/component/NetworkId.hpp"
#include "rtype/replication/component/Owner.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/input/InputMessage.hpp"
#include "rtype/replication/manifest/Manifest.hpp"
#include "rtype/replication/serialization/EntityMapper.hpp"
#include "rtype/replication/serialization/FieldSerializer.hpp"
#include "rtype/replication/snapshot/Snapshot.hpp"

namespace rtype::replication::client {
namespace {
/// @brief Turns every reference into the null entity: the mapper of the validation, which touches no state.
class NullMapper final : public serialization::EntityMapper {
  public:
    NullMapper() = default;
    ~NullMapper() override = default;
    NullMapper(const NullMapper& other) = delete;
    NullMapper& operator=(const NullMapper& other) = delete;
    NullMapper(NullMapper&& other) = delete;
    NullMapper& operator=(NullMapper&& other) = delete;

    [[nodiscard]] std::uint32_t toNetwork(const ecs::Entity /*entity*/) const noexcept override { return 0; }

    [[nodiscard]] ecs::Entity toLocal(const std::uint32_t /*networkId*/) const noexcept override { return {}; }
};

/// @brief Gives the local entity of the NetworkIds the client knows.
class MapMapper final : public serialization::EntityMapper {
  public:
    /// @param entities The `NetworkId → Entity` map; it must outlive the mapper.
    explicit MapMapper(const std::unordered_map<std::uint32_t, ecs::Entity>& entities) noexcept
        : _entities{&entities} {}
    ~MapMapper() override = default;
    MapMapper(const MapMapper& other) = delete;
    MapMapper& operator=(const MapMapper& other) = delete;
    MapMapper(MapMapper&& other) = delete;
    MapMapper& operator=(MapMapper&& other) = delete;

    /// @return 0: the client never writes entities to the network.
    [[nodiscard]] std::uint32_t toNetwork(const ecs::Entity /*entity*/) const noexcept override { return 0; }

    [[nodiscard]] ecs::Entity toLocal(const std::uint32_t networkId) const noexcept override {
        const auto found = _entities->find(networkId);
        return found == _entities->end() ? ecs::Entity{} : found->second;
    }

  private:
    const std::unordered_map<std::uint32_t, ecs::Entity>* _entities;  ///< The `NetworkId → Entity` map.
};

/// @brief Reads one value of a component.
/// @return The bytes of the value, or nothing when the bytes are not a valid value.
[[nodiscard]] std::optional<std::vector<std::byte>> readValue(const ecs::TypeInfo& type,
                                                              const std::span<const std::byte> bytes,
                                                              const serialization::EntityMapper& mapper) {
    std::vector<std::byte> value(type.size);
    network::bytes::ByteReader reader{bytes};
    if (!serialization::readFields(reader, type, value, mapper) || !reader.isFinished()) {
        return std::nullopt;
    }
    return value;
}
}  // namespace

ClientState::ClientState(const network::io::Address& server, const network::NetworkSettings& settings,
                         const ecs::World& world)
    : _server{server}, _settings{settings}, _world{&world}, _time{[] { return network::Clock::now(); }} {}

ClientState::~ClientState() = default;

ManifestStatus ClientState::manifestStatus() const noexcept { return _manifestStatus; }

std::string_view ClientState::mismatch() const noexcept { return _mismatch; }

ConnectionStatus ClientState::connectionStatus() const noexcept { return _connection; }

std::optional<network::DisconnectReason> ClientState::disconnectReason() const noexcept { return _disconnectReason; }

std::optional<network::RejectReason> ClientState::rejectReason() const noexcept { return _rejectReason; }

std::optional<network::ClientId> ClientState::clientId() const noexcept { return _clientId; }

const ReplicationSettings& ClientState::serverSettings() const noexcept { return _receiver.serverSettings(); }

std::uint32_t ClientState::lastAppliedTick() const noexcept { return _lastAppliedTick; }

std::uint32_t ClientState::lastProcessedInput() const noexcept { return _lastProcessedInput; }

std::size_t ClientState::droppedCount() const noexcept { return _dropped; }

std::size_t ClientState::staleCount() const noexcept { return _stale; }

network::client::NetClient& ClientState::netClient() noexcept { return *_client; }

void ClientState::setInput(const component::PlayerInput& input) {
    if (_manifestStatus == ManifestStatus::kAccepted) {
        _input = input;
    }
}

ecs::Entity ClientState::find(const std::uint32_t networkId) const noexcept {
    const auto found = _entities.find(networkId);
    return found == _entities.end() ? ecs::Entity{} : found->second;
}

std::optional<ecs::Entity> ClientState::localPlayer() const noexcept {
    if (!_clientId) {
        return std::nullopt;
    }
    // The lowest NetworkId wins if the game gave two entities to one client: the answer must not depend on the
    // iteration order of the map.
    std::optional<std::uint32_t> best;
    for (const auto& [networkId, entity] : _entities) {
        const auto* owner = _world->getComponents().get<component::Owner>(entity);
        if (owner != nullptr && owner->client == *_clientId && (!best || networkId < *best)) {
            best = networkId;
        }
    }
    if (!best) {
        return std::nullopt;
    }
    return _entities.at(*best);
}

void ClientState::setTimeSource(TimeSource source) { _time = std::move(source); }

void ClientState::connect() {
    _client = std::make_unique<network::client::NetClient>(_server, _settings);
    _connection = ConnectionStatus::kConnecting;
    _client->connect(_time());
}

void ClientState::receive(input::InputLayout& inputs, ecs::Commands& commands) {
    if (!_client) {
        return;
    }
    _client->poll(_time());
    for (const network::client::ClientEvent& clientEvent : _client->takeEvents()) {
        handle(clientEvent, inputs, commands);
    }
}

void ClientState::handle(const network::client::ClientEvent& clientEvent, input::InputLayout& inputs,
                         ecs::Commands& commands) {
    if (const auto* accepted = std::get_if<network::client::ConnectionAccepted>(&clientEvent)) {
        _connection = ConnectionStatus::kConnected;
        _clientId = accepted->client;
        return;
    }
    if (const auto* left = std::get_if<network::client::Disconnected>(&clientEvent)) {
        _connection = ConnectionStatus::kDisconnected;
        _disconnectReason = left->reason;
        return;
    }
    if (const auto* failed = std::get_if<network::client::ConnectFailed>(&clientEvent)) {
        _connection = ConnectionStatus::kFailed;
        _rejectReason = failed->reason;
        return;
    }
    const auto& message = std::get<network::client::ServerMessage>(clientEvent);
    if (message.payload.empty()) {
        ++_dropped;
        return;
    }
    const auto type = static_cast<std::uint8_t>(message.payload.front());
    if (type == static_cast<std::uint8_t>(MessageType::kSnapshot)) {
        static_cast<void>(applySnapshot(message.payload, commands));
        return;
    }
    const bool isManifest = type >= static_cast<std::uint8_t>(MessageType::kManifestComponent) &&
                            type <= static_cast<std::uint8_t>(MessageType::kManifestEnd);
    if (!isManifest || _manifestStatus != ManifestStatus::kWaiting || !_receiver.receive(message.payload)) {
        ++_dropped;
        return;
    }
    if (_receiver.isComplete()) {
        checkManifest(inputs);
    }
}

void ClientState::checkManifest(input::InputLayout& inputs) {
    inputs.freeze();
    manifest::ManifestCheck result = _receiver.check(*_world, inputs);
    if (!result.manifest) {
        _manifestStatus = ManifestStatus::kMismatch;
        _mismatch = std::move(result.mismatch);
        if (_client) {
            _client->disconnect(_time(), network::DisconnectReason::kManifestMismatch);
        }
        return;
    }
    _manifest = std::move(result.manifest);
    _componentSizes = snapshot::computeComponentSizes(*_manifest, *_world);
    _manifestStatus = ManifestStatus::kAccepted;
    if (_client) {
        _client->sendReady();
    }
}

bool ClientState::isLive(const ecs::Entity entity) const {
    return _world->isAlive(entity) || _reserved.contains(entity);
}

bool ClientState::validate(const snapshot::Snapshot& decoded, const manifest::Manifest& manifest) const {
    const auto& registry = _world->getComponents().getRegistry();
    const NullMapper mapper;
    const auto isValid = [&](const std::vector<snapshot::ComponentValue>& values) {
        return std::ranges::all_of(values, [&](const snapshot::ComponentValue& value) {
            const std::optional<ecs::ComponentId> id = manifest.toLocal(value.shortId);
            const ecs::ComponentInfo* info = id ? registry.findById(*id) : nullptr;
            return info != nullptr && readValue(info->type, value.bytes, mapper);
        });
    };
    return std::ranges::all_of(decoded.spawns,
                               [&](const snapshot::SpawnRecord& record) { return isValid(record.components); }) &&
           std::ranges::all_of(decoded.updates,
                               [&](const snapshot::UpdateRecord& record) { return isValid(record.components); }) &&
           std::ranges::all_of(decoded.removals, [&](const snapshot::RemovalRecord& record) {
               return manifest.toLocal(record.shortId).has_value();
           });
}

void ClientState::spawnEntities(const snapshot::Snapshot& decoded, ecs::Commands& commands) {
    for (const snapshot::SpawnRecord& record : decoded.spawns) {
        const auto known = _entities.find(record.networkId);
        if (known != _entities.end() && isLive(known->second)) {
            continue;
        }
        const ecs::Entity entity = commands.spawn().add(component::NetworkId{.value = record.networkId}).getEntity();
        _reserved.insert(entity);
        _entities.insert_or_assign(record.networkId, entity);
    }
}

void ClientState::writeValues(const std::uint32_t networkId, const std::vector<snapshot::ComponentValue>& values,
                              const manifest::Manifest& manifest, ecs::Commands& commands) const {
    const auto entity = _entities.find(networkId);
    if (entity == _entities.end()) {
        return;
    }
    const auto& registry = _world->getComponents().getRegistry();
    const MapMapper mapper{_entities};
    for (const snapshot::ComponentValue& value : values) {
        const std::optional<ecs::ComponentId> id = manifest.toLocal(value.shortId);
        if (!id) {
            continue;
        }
        const ecs::ComponentInfo* info = registry.findById(*id);
        if (info == nullptr) {
            continue;
        }
        if (const std::optional<std::vector<std::byte>> bytes = readValue(info->type, value.bytes, mapper)) {
            static_cast<void>(commands.add(entity->second, *id, *bytes));
        }
    }
}

void ClientState::removeComponents(const snapshot::Snapshot& decoded, const manifest::Manifest& manifest,
                                   ecs::Commands& commands) const {
    for (const snapshot::RemovalRecord& record : decoded.removals) {
        const auto entity = _entities.find(record.networkId);
        const std::optional<ecs::ComponentId> id = manifest.toLocal(record.shortId);
        if (entity != _entities.end() && id) {
            static_cast<void>(commands.remove(entity->second, *id));
        }
    }
}

void ClientState::despawnEntities(const snapshot::Snapshot& decoded, ecs::Commands& commands) {
    for (const std::uint32_t networkId : decoded.despawns) {
        const auto entity = _entities.find(networkId);
        if (entity != _entities.end()) {
            commands.despawn(entity->second);
            _entities.erase(entity);
        }
    }
}

void ClientState::removeUnlisted(const snapshot::Snapshot& decoded, const manifest::Manifest& manifest,
                                 ecs::Commands& commands) {
    std::unordered_set<std::uint32_t> listed;
    for (const snapshot::SpawnRecord& record : decoded.spawns) {
        listed.insert(record.networkId);
    }
    for (auto entity = _entities.begin(); entity != _entities.end();) {
        if (listed.contains(entity->first)) {
            ++entity;
            continue;
        }
        commands.despawn(entity->second);
        entity = _entities.erase(entity);
    }
    const std::span<const ecs::ComponentId> replicated = manifest.components();
    for (const snapshot::SpawnRecord& record : decoded.spawns) {
        const ecs::Entity entity = find(record.networkId);
        std::vector<bool> isListed(replicated.size(), false);
        for (const snapshot::ComponentValue& value : record.components) {
            isListed.at(value.shortId) = true;
        }
        for (std::size_t shortId = 0; shortId < replicated.size(); ++shortId) {
            if (!isListed.at(shortId)) {
                static_cast<void>(commands.remove(entity, replicated[shortId]));  // NOLINT
            }
        }
    }
}

bool ClientState::applySnapshot(const std::span<const std::byte> message, ecs::Commands& commands) {
    if (_manifestStatus != ManifestStatus::kAccepted || !_manifest) {
        ++_stale;
        return false;
    }
    const manifest::Manifest& manifest = *_manifest;
    const std::optional<snapshot::Snapshot> decoded = snapshot::decode(message, _componentSizes);
    if (!decoded || !validate(*decoded, manifest)) {
        ++_dropped;
        return false;
    }
    if (decoded->tick <= _lastAppliedTick) {
        ++_stale;
        return false;
    }
    if (commands.isEmpty()) {
        _reserved.clear();  // what was reserved is alive by now, or gone
    }

    // Every spawn first, so that a field can refer to an entity spawned by the same snapshot.
    spawnEntities(*decoded, commands);
    for (const snapshot::SpawnRecord& record : decoded->spawns) {
        writeValues(record.networkId, record.components, manifest, commands);
    }
    for (const snapshot::UpdateRecord& record : decoded->updates) {
        writeValues(record.networkId, record.components, manifest, commands);
    }
    removeComponents(*decoded, manifest, commands);
    despawnEntities(*decoded, commands);
    if ((decoded->flags & snapshot::kComplete) != 0) {
        removeUnlisted(*decoded, manifest, commands);
    }

    _lastAppliedTick = decoded->tick;
    _lastProcessedInput = decoded->lastProcessedInput;
    return true;
}

std::optional<network::Bytes> ClientState::nextInputMessage(const input::InputLayout& inputs) {
    if (_manifestStatus != ManifestStatus::kAccepted) {
        return std::nullopt;
    }
    const component::PlayerInput input = _input.value_or(component::PlayerInput{});
    _recentInputs.push_back({.sequence = _nextInputSequence++, .buttons = input.buttons, .axes = input.axes});
    while (_recentInputs.size() > input::kMaxInputsPerMessage) {
        _recentInputs.pop_front();
    }
    return input::encodeInput(
        {
            .lastAppliedSnapshot = _lastAppliedTick,
            .inputs = std::vector<input::InputSample>(_recentInputs.begin(), _recentInputs.end()),
        },
        inputs);
}

void ClientState::sendInput(const input::InputLayout& inputs) {
    if (!_client || _connection != ConnectionStatus::kConnected) {
        return;
    }
    if (const std::optional<network::Bytes> message = nextInputMessage(inputs)) {
        static_cast<void>(_client->send(network::Channel::kUnreliable, *message));
        _client->flush(_time());
    }
}
}  // namespace rtype::replication::client
