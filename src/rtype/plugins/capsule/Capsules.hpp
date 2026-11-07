/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Capsules
*/

#pragma once

#include <cstddef>
#include <string_view>
#include <unordered_map>

#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/Export.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::plugins::capsule {
/// @brief The capsules of one world, by id, and the side they are spawned on: the "rtype.Capsules" resource the
/// CapsulePlugin inserts.
/// @details A capsule is defined by its path (define()), then filled (Capsule::addNode(), Capsule::editNode()), in code
/// (addCapsule()) or from its file. Defining it again empties it and bumps its version, keeping its id, so the capsules
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

    /// @return The side spawns happen on.
    [[nodiscard]] ecs::RunMode getRunMode() const noexcept;

    /// @brief Sets the side spawns happen on (the CapsulePlugin does, from the App's run mode).
    void setRunMode(ecs::RunMode mode) noexcept;

    /// @return The number of capsules.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @brief Defines the capsule at `path`: a new one with a root only, or an existing one emptied (a root only) with
    /// the next version. Entities spawned before do not change until they are reloaded.
    /// @param path Its path, normalized here ("actors/bydo.actor"); the extension is the developer's choice.
    /// @return The capsule, to fill.
    /// @throws exceptions::CapsuleException When the path is empty, or has the id of another path.
    Capsule& define(std::string_view path);

    /// @return The capsule with this id, or nullptr.
    [[nodiscard]] const Capsule* find(CapsuleId id) const noexcept;

    /// @return The capsule at this path, or nullptr.
    [[nodiscard]] const Capsule* find(std::string_view path) const;

    /// @return The capsule with this id, to change its nodes. Entities spawned before do not change.
    /// @throws exceptions::CapsuleException When no capsule has this id.
    [[nodiscard]] Capsule& edit(CapsuleId id);

    /// @return Whether a spawn on this registry's side creates a node of this side (ecs::runsIn()).
    [[nodiscard]] bool loads(ecs::Side side) const noexcept;

    /// @return Whether a spawn on this registry's side keeps a component with this presence: false for a server-only
    /// one on a client, and for a client-only one on a dedicated server.
    [[nodiscard]] bool keeps(ecs::Presence presence) const noexcept;

  private:
    std::unordered_map<CapsuleId, Capsule> _capsules;  ///< Capsules by id; a node-based map keeps their addresses.
    ecs::RunMode _mode{ecs::RunMode::kStandalone};     ///< The side spawns happen on.
};

RTYPE_ECS_RESOURCE(Capsules, "rtype.Capsules");
}  // namespace rtype::plugins::capsule
