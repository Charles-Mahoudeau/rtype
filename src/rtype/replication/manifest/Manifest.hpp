/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Manifest
*/

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/replication/Export.hpp"
#include "rtype/replication/ReplicationSettings.hpp"
#include "rtype/replication/input/InputLayout.hpp"

namespace rtype::ecs {
class World;
}  // namespace rtype::ecs

namespace rtype::replication::manifest {
/// @brief A replicated component as the manifest lists it.
struct ComponentEntry {
    std::string name;             ///< The component's name.
    std::uint64_t layoutHash{0};  ///< The component's layout hash.

    friend bool operator==(const ComponentEntry& left, const ComponentEntry& right) noexcept = default;
};

/// @brief The list of everything the server replicates, with the hash of each layout.
///
/// @details A component's position in the list, sorted by name, is its short id: a `u16` sent on the wire instead of
/// the name. Short ids are not the world's ComponentIds, which depend on registration order: each side maps a short
/// id to its own ComponentId by name.
class RTYPE_REPLICATION_API Manifest {
  public:
    /// @brief Lists the replicated components of a world and the input actions of a layout. The caller freezes the
    /// layout, so that no action is declared once the manifest exists.
    /// @param world The world, once every plugin registered its types.
    /// @param inputs The game's input actions.
    /// @throws exceptions::ManifestException When a replicated component cannot be serialized, has a name longer than
    /// 255 bytes, or when there are more than 65 535 components or 256 actions.
    [[nodiscard]] static Manifest build(const ecs::World& world, const input::InputLayout& inputs);

    /// @brief Writes the welcome messages: one per component, one per action, then the end with the settings.
    /// @param settings The settings the client needs.
    /// @return The messages in order, to send on the reliable channel.
    [[nodiscard]] std::vector<network::Bytes> encode(const ReplicationSettings& settings) const;

    /// @return The hash of every entry in order (FNV-1a 64).
    [[nodiscard]] std::uint64_t hash() const noexcept;

    /// @return The components by short id.
    [[nodiscard]] std::span<const ComponentEntry> componentEntries() const noexcept;

    /// @return The input actions by index.
    [[nodiscard]] std::span<const input::InputAction> actions() const noexcept;

    /// @return The local ids of the components by short id.
    [[nodiscard]] std::span<const ecs::ComponentId> components() const noexcept;

    /// @param shortId A short id of the manifest.
    /// @return The local component, or nullopt for a short id that is not in the manifest.
    [[nodiscard]] std::optional<ecs::ComponentId> toLocal(std::uint16_t shortId) const noexcept;

    /// @param id A local component.
    /// @return Its short id, or nullopt for a component that is not in the manifest.
    [[nodiscard]] std::optional<std::uint16_t> toShort(ecs::ComponentId id) const noexcept;

    /// @brief Hashes entries the way the manifest does.
    /// @param components The components by short id.
    /// @param actions The actions by index.
    /// @return The manifest hash.
    [[nodiscard]] static std::uint64_t computeHash(std::span<const ComponentEntry> components,
                                                   std::span<const input::InputAction> actions) noexcept;

  private:
    std::vector<ComponentEntry> _entries;                         ///< By short id.
    std::vector<input::InputAction> _actions;                     ///< By index.
    std::vector<ecs::ComponentId> _components;                    ///< Local ids by short id.
    std::unordered_map<ecs::ComponentId, std::uint16_t> _shorts;  ///< Short ids by local id.
    std::uint64_t _hash{0};                                       ///< Of every entry.
};

/// @brief The result of ManifestReceiver::check().
struct ManifestCheck {
    std::optional<Manifest> manifest;  ///< Set on success, mapped to the local world.
    std::string mismatch;              ///< Set on failure: the first difference, for the player.
};
}  // namespace rtype::replication::manifest
