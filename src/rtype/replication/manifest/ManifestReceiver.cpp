/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ManifestReceiver
*/

#include "rtype/replication/manifest/ManifestReceiver.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>

#include "rtype/ecs/world/World.hpp"
#include "rtype/network/bytes/Overloaded.hpp"
#include "rtype/replication/ReplicationSettings.hpp"
#include "rtype/replication/exceptions/ReplicationExceptions.hpp"
#include "rtype/replication/input/InputLayout.hpp"
#include "rtype/replication/manifest/Manifest.hpp"
#include "rtype/replication/manifest/ManifestMessages.hpp"

namespace rtype::replication::manifest {
namespace {
/// @brief Tells whether the keys of `entries` are 0, 1, 2... and the names strictly increase (sorted and unique).
template <typename Key, typename Entry>
[[nodiscard]] bool isContiguousAndSorted(const std::map<Key, Entry>& entries) {
    std::size_t expected = 0;
    const std::string* previous = nullptr;
    for (const auto& [key, entry] : entries) {
        if (static_cast<std::size_t>(key) != expected) {
            return false;
        }
        if (previous != nullptr && !(*previous < entry.name)) {
            return false;
        }
        previous = &entry.name;
        ++expected;
    }
    return true;
}

/// @brief Describes the first difference between the components of the server and those of the client.
/// @return The message, or an empty string when the lists are equal.
[[nodiscard]] std::string describeComponents(const std::map<std::uint16_t, ComponentEntry>& server,
                                             const std::span<const ComponentEntry> client) {
    auto serverIt = server.begin();
    auto clientIt = client.begin();
    while (serverIt != server.end() || clientIt != client.end()) {
        const bool serverOnly =
            clientIt == client.end() || (serverIt != server.end() && serverIt->second.name < clientIt->name);
        const bool clientOnly = !serverOnly && (serverIt == server.end() || clientIt->name < serverIt->second.name);
        if (serverOnly) {
            return std::format("`{}` exists on the server but not on the client: update your game",
                               serverIt->second.name);
        }
        if (clientOnly) {
            return std::format("`{}` exists on the client but not on the server: update your game", clientIt->name);
        }
        if (serverIt->second.layoutHash != clientIt->layoutHash) {
            return std::format("`{}` differs between client and server: update your game", clientIt->name);
        }
        ++serverIt;
        ++clientIt;
    }
    return {};
}

/// @brief Describes the first difference between the input actions of the server and those of the client.
/// @return The message, or an empty string when the lists are equal.
[[nodiscard]] std::string describeActions(const std::map<std::uint8_t, input::InputAction>& server,
                                          const std::span<const input::InputAction> client) {
    auto serverIt = server.begin();
    auto clientIt = client.begin();
    while (serverIt != server.end() || clientIt != client.end()) {
        const bool serverOnly =
            clientIt == client.end() || (serverIt != server.end() && serverIt->second.name < clientIt->name);
        const bool clientOnly = !serverOnly && (serverIt == server.end() || clientIt->name < serverIt->second.name);
        if (serverOnly) {
            return std::format("The input action `{}` exists on the server but not on the client: update your game",
                               serverIt->second.name);
        }
        if (clientOnly) {
            return std::format("The input action `{}` exists on the client but not on the server: update your game",
                               clientIt->name);
        }
        if (serverIt->second.kind != clientIt->kind) {
            return std::format("The input action `{}` differs between client and server: update your game",
                               clientIt->name);
        }
        ++serverIt;
        ++clientIt;
    }
    return {};
}
}  // namespace

const ReplicationSettings& ManifestReceiver::serverSettings() const noexcept { return _settings; }

bool ManifestReceiver::isComplete() const noexcept { return _complete; }

bool ManifestReceiver::receive(const std::span<const std::byte> message) {
    if (_complete) {
        return false;
    }
    const std::optional<ManifestCodec::Message> decoded = ManifestCodec::decode(message);
    if (!decoded) {
        return false;
    }
    return std::visit(
        network::bytes::Overloaded{
            [this](const ManifestComponent& component) {
                return _components
                    .try_emplace(component.shortId,
                                 ComponentEntry{.name = component.name, .layoutHash = component.layoutHash})
                    .second;
            },
            [this](const ManifestInputAction& action) {
                return _actions.try_emplace(action.index, input::InputAction{.name = action.name, .kind = action.kind})
                    .second;
            },
            [this](const ManifestEnd& end) {
                if (end.entryCount != _components.size() + _actions.size() || !isContiguousAndSorted(_components) ||
                    !isContiguousAndSorted(_actions)) {
                    return false;
                }
                _hash = end.hash;
                _settings = {
                    .tickRate = end.tickRate,
                    .snapshotRate = end.snapshotRate,
                    .reconnectionGraceSeconds = end.reconnectionGraceSeconds,
                };
                _complete = true;
                return true;
            },
        },
        *decoded);
}

ManifestCheck ManifestReceiver::check(const ecs::World& world, const input::InputLayout& inputs) const {
    if (!_complete) {
        return {.manifest = std::nullopt, .mismatch = "The manifest of the server is incomplete"};
    }
    std::optional<Manifest> local;
    try {
        local = Manifest::build(world, inputs);
    } catch (const exceptions::ManifestException& exception) {
        return {.manifest = std::nullopt, .mismatch = exception.what()};
    }
    if (local->hash() == _hash) {
        return {.manifest = std::move(local), .mismatch = {}};
    }
    std::string mismatch = describeComponents(_components, local->componentEntries());
    if (mismatch.empty()) {
        mismatch = describeActions(_actions, local->actions());
    }
    if (mismatch.empty()) {
        mismatch = "The manifest of the server differs from the one of the client: update your game";
    }
    return {.manifest = std::nullopt, .mismatch = std::move(mismatch)};
}
}  // namespace rtype::replication::manifest
