/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentRegistry
*/

#include "ComponentRegistry.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <span>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/NameHash.hpp"
#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"

namespace rtype::ecs {
namespace {

[[nodiscard]] std::string_view toString(StorageKind storage) noexcept {
    switch (storage) {
        case StorageKind::kSparseSet:
            return "sparseSet";
        case StorageKind::kTable:
            return "table";
    }
    return "unknown";
}

[[nodiscard]] std::string_view toString(Presence presence) noexcept {
    switch (presence) {
        case Presence::kBoth:
            return "both";
        case Presence::kServerOnly:
            return "serverOnly";
        case Presence::kClientOnly:
            return "clientOnly";
    }
    return "unknown";
}

[[nodiscard]] std::string_view toString(ReplicationMode replication) noexcept {
    switch (replication) {
        case ReplicationMode::kNone:
            return "none";
        case ReplicationMode::kEveryChange:
            return "everyChange";
        case ReplicationMode::kSpawnOnly:
            return "spawnOnly";
    }
    return "unknown";
}

/// @return An ErrorKind::kOptionsMismatch error naming the option and both values.
template <typename Option>
[[nodiscard]] tl::unexpected<Error> optionsMismatch(std::string_view component, std::string_view option,
                                                    Option registered, Option requested) {
    return fail(ErrorKind::kOptionsMismatch, std::format("Component '{}' is already registered with {} '{}', not '{}'",
                                                         component, option, toString(registered), toString(requested)));
}

/// @brief Decides what registering `requested` again means when `registered` has the same name hash.
/// @return The registered id when both are the same component, or the reason they conflict.
[[nodiscard]] Result<ComponentId> checkSameComponent(const ComponentInfo& registered, const ComponentInfo& requested) {
    if (registered.name != requested.name) {
        return fail(ErrorKind::kNameCollision,
                    std::format("Components '{}' and '{}' have the same name hash", registered.name, requested.name));
    }
    if (registered.layoutHash != requested.layoutHash) {
        return fail(ErrorKind::kLayoutMismatch,
                    std::format("Component '{}' is already registered with another layout (size, alignment or fields)",
                                requested.name));
    }
    if (registered.storage != requested.storage) {
        return optionsMismatch(requested.name, "storage", registered.storage, requested.storage);
    }
    if (registered.presence != requested.presence) {
        return optionsMismatch(requested.name, "presence", registered.presence, requested.presence);
    }
    if (registered.replication != requested.replication) {
        return optionsMismatch(requested.name, "replication", registered.replication, requested.replication);
    }
    return registered.id;
}

}  // namespace

std::size_t ComponentRegistry::getCount() const noexcept { return _infos.size(); }

std::span<const ComponentId> ComponentRegistry::getReplicatedIds() const noexcept { return _replicatedIds; }

Result<ComponentId> ComponentRegistry::registerComponent(ComponentInfo info) {
    if (Result<void> valid = validate(info); !valid) {
        return tl::unexpected<Error>{std::move(valid.error())};
    }
    info.layoutHash = computeLayoutHash(info);
    const std::uint64_t nameHash = hashName(info.name);
    if (const ComponentInfo* registered = findByHash(nameHash); registered != nullptr) {
        return checkSameComponent(*registered, info);
    }
    if (_infos.size() >= kInvalidComponentId) {
        return fail(ErrorKind::kInvalidComponent, std::format("Component '{}': no component id left", info.name));
    }
    const auto id = static_cast<ComponentId>(_infos.size());
    info.id = id;
    const bool replicated = info.replication != ReplicationMode::kNone;
    _infos.push_back(std::move(info));
    _idsByHash.emplace(nameHash, id);
    if (replicated) {
        _replicatedIds.push_back(id);
    }
    return id;
}

const ComponentInfo* ComponentRegistry::findById(ComponentId id) const noexcept {
    if (id >= _infos.size()) {
        return nullptr;
    }
    // In range: checked above, so the element is reached through an iterator rather than a throwing at().
    return &*std::next(_infos.begin(), static_cast<std::ptrdiff_t>(id));
}

const ComponentInfo* ComponentRegistry::findByName(std::string_view name) const noexcept {
    const ComponentInfo* info = findByHash(hashName(name));
    return info != nullptr && info->name == name ? info : nullptr;
}

const ComponentInfo* ComponentRegistry::findByHash(std::uint64_t nameHash) const noexcept {
    const auto found = _idsByHash.find(nameHash);
    return found == _idsByHash.end() ? nullptr : findById(found->second);
}
}  // namespace rtype::ecs
