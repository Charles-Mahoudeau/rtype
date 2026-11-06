/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Prefabs
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/prefab/ComponentValues.hpp"
#include "rtype/ecs/prefab/Prefab.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::ecs {
/// @brief The prefabs of one world, by id and by name, and the side they are spawned on.
/// @details Prefabs are added empty and filled through edit(); a child is added with addChild(), which refuses a
/// prefab that would contain itself. Pointers returned by the lookups stay valid for the registry's lifetime (a
/// deque keeps their addresses). The run mode (set by the App) decides which components a spawn keeps: a client
/// leaves out server-only ones, a dedicated server client-only ones, standalone keeps them all.
/// @code
/// rtype::ecs::Prefabs& prefabs = world.getPrefabs();
/// const rtype::ecs::PrefabId turret = prefabs.add("game.Turret");
/// prefabs.edit(turret).getValues().set(world.getComponents(), Health{.value = 5});
/// @endcode
class RTYPE_ECS_API Prefabs {
  public:
    Prefabs() = default;
    ~Prefabs() = default;
    Prefabs(const Prefabs& other) = delete;
    Prefabs& operator=(const Prefabs& other) = delete;
    Prefabs(Prefabs&& other) noexcept = default;
    Prefabs& operator=(Prefabs&& other) noexcept = default;

    /// @return The side spawns happen on.
    [[nodiscard]] RunMode getRunMode() const noexcept;

    /// @brief Sets the side spawns happen on (the App does, from its run mode).
    void setRunMode(RunMode mode) noexcept;

    /// @return The number of prefabs; ids go from 0 to this count minus one.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @brief Adds an empty prefab.
    /// @param name Its stable name, such as "game.Bydo".
    /// @return Its id.
    /// @throws exceptions::PrefabException When the name is empty, already taken, or has the hash of another name.
    PrefabId add(std::string name);

    /// @return The prefab with this id, or nullptr.
    [[nodiscard]] const Prefab* findById(PrefabId id) const noexcept;

    /// @return The prefab with this name, or nullptr.
    [[nodiscard]] const Prefab* findByName(std::string_view name) const noexcept;

    /// @return The prefab with this id, to change its values. Entities already spawned from it do not change.
    /// @throws exceptions::PrefabException When no prefab has this id.
    [[nodiscard]] Prefab& edit(PrefabId id);

    /// @brief Adds a child to a prefab: spawned with it and parented to it, with `overrides` replacing its values.
    /// @throws exceptions::PrefabException When either id is unknown, or the child contains the parent (or is it): a
    /// prefab cannot contain itself.
    void addChild(PrefabId parent, PrefabId child, ComponentValues overrides = {});

    /// @return Whether a spawn on this registry's side keeps a component of this type: false for a server-only one on a
    /// client, and for a client-only one on a dedicated server.
    [[nodiscard]] bool keeps(const TypeInfo& type) const noexcept;

  private:
    /// @return Whether `target` is `root` or one of its children, at any depth.
    [[nodiscard]] bool contains(PrefabId root, PrefabId target) const noexcept;

    std::deque<Prefab> _prefabs;                             ///< Prefabs by id; a deque keeps their addresses.
    std::unordered_map<std::uint64_t, PrefabId> _idsByHash;  ///< Ids by name hash.
    RunMode _mode{RunMode::kStandalone};                     ///< The side spawns happen on.
};
}  // namespace rtype::ecs
