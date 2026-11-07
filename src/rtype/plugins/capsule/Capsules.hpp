/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Capsules
*/

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>

#include "core/asset/AssetId.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/Export.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::plugins::capsule {
/// @brief The capsules of one world, by id, and the side they are spawned on: the "rtype.Capsules" resource the
/// CapsulePlugin inserts.
/// @details A capsule is built (Capsule::addNode(), Capsule::editNode()), in code or from its file, then defined
/// (define()). Defining it again replaces its nodes and bumps its version, keeping its id and address, so the capsules
/// that instance it stay valid: hot reload. Pointers returned by the lookups stay valid for the registry's lifetime.
/// The run mode (the App's, set by the CapsulePlugin) decides which nodes a spawn creates (loads()) and which
/// components it keeps (keeps()).
class RTYPE_PLUGIN_CAPSULE_API Capsules {
  public:
    Capsules() = default;
    ~Capsules() = default;
    Capsules(const Capsules& other) = delete;
    Capsules& operator=(const Capsules& other) = delete;
    Capsules(Capsules&& other) noexcept = default;
    Capsules& operator=(Capsules&& other) noexcept = default;

    /// @brief Sets the side spawns happen on (the CapsulePlugin does, from the App's run mode).
    void setRunMode(ecs::RunMode mode) noexcept;

    /// @return The number of capsules.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @brief Adds a capsule, or replaces the nodes of the one with its id and bumps its version, in one step (hot
    /// reload: its address stays; a file that fails to read never reaches it, so the old version stays). Entities
    /// spawned before do not change until they are reloaded.
    /// @return The capsule stored.
    /// @throws exceptions::CapsuleException When its id is another path's (a hash collision); nothing changes then.
    Capsule& define(Capsule capsule);

    /// @return The capsule with this id, or nullptr.
    [[nodiscard]] const Capsule* find(CapsuleId id) const noexcept;

    /// @return The capsule at this path, or nullptr.
    [[nodiscard]] const Capsule* find(std::string_view path) const;

    /// @return The capsule with this id, to change its nodes. Entities spawned before do not change.
    /// @throws exceptions::CapsuleException When no capsule has this id.
    [[nodiscard]] Capsule& edit(CapsuleId id);

    /// @brief Remembers the path an asset id was read from, so a capsule written back names the asset by its path (an
    /// id is a hash: it cannot give the path back).
    void rememberAssetPath(core::asset::AssetId id, std::string path);

    /// @return The path an asset id was read from, or nullptr if none was remembered.
    [[nodiscard]] const std::string* findAssetPath(core::asset::AssetId id) const noexcept;

    /// @return Whether a spawn on this registry's side creates a node of this side (ecs::runsIn()).
    [[nodiscard]] bool loads(ecs::Side side) const noexcept;

    /// @return Whether a spawn on this registry's side keeps a component with this presence: false for a server-only
    /// one on a client, and for a client-only one on a dedicated server.
    [[nodiscard]] bool keeps(ecs::Presence presence) const noexcept;

  private:
    std::unordered_map<CapsuleId, Capsule> _capsules;  ///< Capsules by id; a node-based map keeps their addresses.
    std::unordered_map<core::asset::AssetId, std::string> _assetPaths;  ///< The paths asset ids were read from.
    ecs::RunMode _mode{ecs::RunMode::kStandalone};                      ///< The side spawns happen on.
};

RTYPE_ECS_RESOURCE(Capsules, "rtype.Capsules");
}  // namespace rtype::plugins::capsule
