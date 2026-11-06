/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Prefab
*/

#pragma once

#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/prefab/ComponentValues.hpp"

namespace rtype::ecs {
/// @brief Dense index of a prefab in its world's Prefabs, local to one world. Use the prefab's name across worlds.
using PrefabId = std::uint32_t;

/// @brief The id of no prefab.
inline constexpr PrefabId kInvalidPrefabId = std::numeric_limits<PrefabId>::max();

/// @brief A child of a prefab: another prefab, spawned with it and parented to it, with values of its own.
struct PrefabChild {
    PrefabId prefab{kInvalidPrefabId};  ///< The child's prefab.
    ComponentValues overrides;          ///< Values replacing the child prefab's (its Transform relative to the parent).
};

/// @brief A named template of an entity: the component values each entity spawned from it gets, copied, and the
/// child prefabs spawned with it.
/// @details Instances are copies: changing a prefab only changes the entities spawned after. Its children are added
/// through Prefabs::addChild(), which refuses a prefab that would contain itself.
class RTYPE_ECS_API Prefab {
  public:
    /// @param id Its id in its Prefabs.
    /// @param name Its stable name, such as "game.Bydo".
    Prefab(PrefabId id, std::string name);
    ~Prefab() = default;
    Prefab(const Prefab& other) = delete;
    Prefab& operator=(const Prefab& other) = delete;
    Prefab(Prefab&& other) noexcept = default;
    Prefab& operator=(Prefab&& other) noexcept = default;

    /// @return Its id in its Prefabs.
    [[nodiscard]] PrefabId getId() const noexcept;

    /// @return Its stable name.
    [[nodiscard]] std::string_view getName() const noexcept;

    /// @return The values each spawned entity gets.
    [[nodiscard]] const ComponentValues& getValues() const noexcept;

    /// @return The values each spawned entity gets, to set or remove some.
    [[nodiscard]] ComponentValues& getValues() noexcept;

    /// @return The child prefabs spawned with it, in the order they were added.
    [[nodiscard]] std::span<const PrefabChild> getChildren() const noexcept;

  private:
    friend class Prefabs;  ///< Adds the children, once it checked they make no cycle.

    PrefabId _id;                        ///< Its id in its Prefabs.
    std::string _name;                   ///< Its stable name.
    ComponentValues _values;             ///< What each spawned entity gets.
    std::vector<PrefabChild> _children;  ///< The child prefabs, in the order they were added.
};
}  // namespace rtype::ecs
