/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PrefabConfig
*/

#pragma once

#include <cstddef>
#include <span>
#include <string_view>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/prefab/Prefab.hpp"

namespace rtype::ecs {
class World;

/// @brief Fills a prefab just added, chained: what App::addPrefab() returns.
/// @details Each call changes the prefab in its world's Prefabs at once; a value set again replaces the previous one.
/// @code
/// app.addPrefab("game.Boss")
///     .with(Health{.value = 50})
///     .withChild("game.Turret", Transform{.position = {-40.F, 0.F}})
///     .withChild("game.Turret", Transform{.position = {40.F, 0.F}});
/// @endcode
class RTYPE_ECS_API PrefabConfig {
  public:
    /// @param world The world holding the prefab; it must outlive this configuration.
    /// @param id The prefab, registered in the world's Prefabs.
    PrefabConfig(World& world, PrefabId id) noexcept;
    ~PrefabConfig() = default;
    PrefabConfig(const PrefabConfig& other) noexcept = default;
    PrefabConfig& operator=(const PrefabConfig& other) noexcept = default;
    PrefabConfig(PrefabConfig&& other) noexcept = default;
    PrefabConfig& operator=(PrefabConfig&& other) noexcept = default;

    /// @return The prefab being filled.
    [[nodiscard]] PrefabId getId() const noexcept;

    /// @brief Gives the prefab a C++ component value (ComponentValues::set()).
    template <Component T>
    PrefabConfig& with(T value);

    /// @brief Gives the prefab a copy of a component value, by id and bytes (ComponentValues::set()).
    /// @throws exceptions::PrefabException When the id is unknown, Parent or Children, or `value` has the wrong size.
    PrefabConfig& with(ComponentId id, std::span<const std::byte> value);

    /// @brief Adds a child prefab, registered before, with C++ component values replacing its own
    /// (Prefabs::addChild()).
    /// @throws exceptions::PrefabException When no prefab has this name, or the child contains this prefab.
    template <Component... Overrides>
    PrefabConfig& withChild(std::string_view child, Overrides... overrides);

    /// @brief Adds a child prefab by id, with C++ component values replacing its own (Prefabs::addChild()).
    /// @throws exceptions::PrefabException When no prefab has this id, or the child contains this prefab.
    template <Component... Overrides>
    PrefabConfig& withChild(PrefabId child, Overrides... overrides);

  private:
    /// @return The id of the prefab named `name`.
    /// @throws exceptions::PrefabException When no prefab has this name.
    [[nodiscard]] PrefabId findChild(std::string_view name) const;

    World* _world;  ///< The world holding the prefab.
    PrefabId _id;   ///< The prefab being filled.
};
}  // namespace rtype::ecs

#define RTYPE_PREFAB_CONFIG_INCLUDING_TPP
#include "PrefabConfig.tpp"  // IWYU pragma: export
#undef RTYPE_PREFAB_CONFIG_INCLUDING_TPP
