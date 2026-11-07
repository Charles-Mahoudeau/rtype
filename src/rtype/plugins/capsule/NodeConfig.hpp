/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NodeConfig
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/Export.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/system/Side.hpp"

namespace rtype::ecs {
class App;
class World;
}  // namespace rtype::ecs

namespace rtype::plugins::capsule {
class Capsules;

/// @brief Fills one node of a capsule, chained: what addCapsule() returns for the root, and child() for a child.
/// @details Each call changes the capsule in its world's Capsules at once; a value set again replaces the previous one.
/// child() returns the new child's configuration, so a chain goes down the tree; keep a configuration to add siblings.
/// @code
/// rtype::plugins::capsule::NodeConfig boss = rtype::plugins::capsule::addCapsule(app, "actors/boss.actor");
/// boss.with(Health{.value = 50});
/// boss.child("LeftTurret").instance("actors/turret.actor").with(Transform{.position = {-40.F, 0.F}});
/// boss.child("RightTurret").instance("actors/turret.actor").with(Transform{.position = {40.F, 0.F}});
/// @endcode
class RTYPE_PLUGIN_CAPSULE_API NodeConfig {
  public:
    /// @param world The world holding the capsule in its Capsules resource; it must outlive this configuration.
    /// @param capsule The capsule, defined in the world's Capsules.
    /// @param node The index of the node in the capsule.
    NodeConfig(ecs::World& world, CapsuleId capsule, std::uint32_t node) noexcept;
    ~NodeConfig() = default;
    NodeConfig(const NodeConfig& other) noexcept = default;
    NodeConfig& operator=(const NodeConfig& other) noexcept = default;
    NodeConfig(NodeConfig&& other) noexcept = default;
    NodeConfig& operator=(NodeConfig&& other) noexcept = default;

    /// @return The capsule being filled.
    [[nodiscard]] CapsuleId getCapsule() const noexcept;

    /// @return The index of the node being filled.
    [[nodiscard]] std::uint32_t getNode() const noexcept;

    /// @brief Gives the node a C++ component value (ComponentValues::set()).
    template <ecs::Component T>
    NodeConfig& with(T value);

    /// @brief Gives the node a copy of a component value, by id and bytes (ComponentValues::set()).
    /// @throws exceptions::CapsuleException When the id is unknown, Parent or Children, or `value` has the wrong size.
    NodeConfig& with(ecs::ComponentId id, std::span<const std::byte> value);

    /// @brief Adds a child node (Capsule::addNode()).
    /// @return The child's configuration.
    /// @throws exceptions::CapsuleException When the name is refused.
    [[nodiscard]] NodeConfig child(std::string name);

    /// @brief Makes the node spawn another capsule: its values replace that capsule root's, its children are added
    /// under it. The capsule is looked up when this one is spawned, so it may be defined later.
    NodeConfig& instance(std::string_view path);

    /// @brief Sets the side the node is spawned on (the server by default).
    /// @throws exceptions::CapsuleException On the root, which is spawned on every side.
    NodeConfig& onSide(ecs::Side side);

  private:
    /// @return The node being filled.
    [[nodiscard]] CapsuleNode& editNode() const;

    /// @return The world's capsules.
    [[nodiscard]] Capsules& getCapsules() const;

    ecs::World* _world;   ///< The world holding the capsule.
    CapsuleId _capsule;   ///< The capsule being filled.
    std::uint32_t _node;  ///< The index of the node being filled.
};

/// @brief Defines the capsule at `path` in the App's world (Capsules::define(): a new one, or an existing one emptied
/// with the next version), to fill from its root.
/// @throws exceptions::CapsuleException When the CapsulePlugin was not added, or the path is refused.
RTYPE_PLUGIN_CAPSULE_API NodeConfig addCapsule(ecs::App& app, std::string_view path);
}  // namespace rtype::plugins::capsule

#define RTYPE_NODE_CONFIG_INCLUDING_TPP
#include "NodeConfig.tpp"  // IWYU pragma: export
#undef RTYPE_NODE_CONFIG_INCLUDING_TPP
