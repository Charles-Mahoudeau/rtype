/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Capsule
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/capsule/ComponentValues.hpp"
#include "plugins/capsule/Export.hpp"
#include "rtype/ecs/system/Side.hpp"

namespace rtype::plugins::capsule {
/// @brief Names a capsule: the AssetId of its path ("actors/bydo.actor"), the same in every process and the id its file
/// has.
using CapsuleId = core::asset::AssetId;

/// @brief The parent of the root node: none.
inline constexpr std::uint32_t kNoParent = std::numeric_limits<std::uint32_t>::max();

/// @brief A capsule another one spawns at one of its nodes: its id, and its path for messages and files.
struct CapsuleRef {
    CapsuleId id;      ///< The id of its path.
    std::string path;  ///< Its path, normalized.
};

/// @brief One node of a capsule: an entity to spawn, with its components, under its parent node.
struct CapsuleNode {
    std::string name;                    ///< Its name among its parent's children; empty for the root.
    std::uint32_t parent{kNoParent};     ///< The index of its parent node; kNoParent for the root.
    ecs::Side side{ecs::Side::kServer};  ///< Where it is spawned (ecs::runsIn()); the root is spawned everywhere.
    std::optional<CapsuleRef> instance;  ///< The capsule it spawns instead of a plain entity, if any.
    ComponentValues values;              ///< Its components; on an instance, they replace the instanced root's.
};

/// @brief An entity with components and child entities, described once and spawned by copy: an actor, a scene, any
/// tree of entities. `.capsule`, `.scene` and `.actor` files all describe one.
/// @details Its nodes form a flat list, a parent always before its children, node 0 the root: the structure of its
/// file, one node after the other. A node is a plain entity, or another capsule (`instance`), whose root takes the
/// node's values over its own and gets the node's children too. Spawning creates one entity per node and parents each
/// to its parent's entity. Capsules::define() replaces a capsule's nodes and bumps its version, for a reload.
/// @code
/// rtype::plugins::capsule::Capsule boss{"actors/boss.actor"};
/// boss.editNode(0).values.set(components, Health{.value = 50});
/// const std::uint32_t turret = boss.addNode("LeftTurret", 0);
/// boss.editNode(turret).instance = rtype::plugins::capsule::CapsuleRef{.id = turretId, .path = "actors/turret.actor"};
/// capsules.define(std::move(boss));
/// @endcode
class RTYPE_PLUGIN_CAPSULE_API Capsule {
  public:
    /// @brief A capsule with a root only.
    /// @param path Its path ("actors/bydo.actor"), normalized here; its id is the AssetId of the normalized path.
    /// @throws exceptions::CapsuleException When the path is empty.
    explicit Capsule(std::string_view path);
    ~Capsule() = default;
    Capsule(const Capsule& other) = delete;
    Capsule& operator=(const Capsule& other) = delete;
    Capsule(Capsule&& other) noexcept = default;
    Capsule& operator=(Capsule&& other) noexcept = default;

    /// @return The id of its path.
    [[nodiscard]] CapsuleId getId() const noexcept;

    /// @return Its path, normalized.
    [[nodiscard]] std::string_view getPath() const noexcept;

    /// @return How many times it replaced an earlier definition (Capsules::define()): 0 for the first one.
    [[nodiscard]] std::uint32_t getVersion() const noexcept;

    /// @return Its nodes, a parent before its children, node 0 the root.
    [[nodiscard]] std::span<const CapsuleNode> getNodes() const noexcept;

    /// @return The node at `index`, to set its values, side or instance.
    /// @throws exceptions::CapsuleException When there is no node at `index`.
    [[nodiscard]] CapsuleNode& editNode(std::size_t index);

    /// @brief Adds a node under `parent`.
    /// @param name Its name among its parent's children: not empty, without '/', not "." nor "..", and unique there.
    /// @return Its index.
    /// @throws exceptions::CapsuleException When `parent` is not a node, or the name is refused.
    std::uint32_t addNode(std::string name, std::uint32_t parent);

    /// @return The index of the node at `path` ("Boss/LeftTurret", "." or "" for the root), or nothing.
    [[nodiscard]] std::optional<std::uint32_t> findNode(std::string_view path) const;

    /// @return The path of a node from the root ("Boss/LeftTurret"), "." for the root; `index` below the node count.
    [[nodiscard]] std::string getNodePath(std::size_t index) const;

  private:
    friend class Capsules;  ///< Replaces its nodes and bumps its version.

    CapsuleId _id;                    ///< The id of its path.
    std::string _path;                ///< Its path, normalized.
    std::uint32_t _version{0};        ///< How many times it replaced an earlier definition.
    std::vector<CapsuleNode> _nodes;  ///< Its nodes, node 0 the root.
};
}  // namespace rtype::plugins::capsule
