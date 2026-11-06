/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ActorConfig
*/

#pragma once

#include <cstddef>
#include <span>
#include <string_view>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/actor/Actor.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::ecs {
class World;

/// @brief Fills an actor just added, chained: what App::addActor() returns.
/// @details Each call changes the actor in its world's Actors at once; a value set again replaces the previous one.
/// @code
/// app.addActor("game.Boss")
///     .with(Health{.value = 50})
///     .withChild("game.Turret", Transform{.position = {-40.F, 0.F}})
///     .withChild("game.Turret", Transform{.position = {40.F, 0.F}});
/// @endcode
class RTYPE_ECS_API ActorConfig {
  public:
    /// @param world The world holding the actor; it must outlive this configuration.
    /// @param id The actor, registered in the world's Actors.
    ActorConfig(World& world, ActorId id) noexcept;
    ~ActorConfig() = default;
    ActorConfig(const ActorConfig& other) noexcept = default;
    ActorConfig& operator=(const ActorConfig& other) noexcept = default;
    ActorConfig(ActorConfig&& other) noexcept = default;
    ActorConfig& operator=(ActorConfig&& other) noexcept = default;

    /// @return The actor being filled.
    [[nodiscard]] ActorId getId() const noexcept;

    /// @brief Gives the actor a C++ component value (ComponentValues::set()).
    template <Component T>
    ActorConfig& with(T value);

    /// @brief Gives the actor a copy of a component value, by id and bytes (ComponentValues::set()).
    /// @throws exceptions::ActorException When the id is unknown, Parent or Children, or `value` has the wrong size.
    ActorConfig& with(ComponentId id, std::span<const std::byte> value);

    /// @brief Adds a child actor, registered before, with C++ component values replacing its own
    /// (Actors::addChild()).
    /// @throws exceptions::ActorException When no actor has this name, or the child contains this actor.
    template <Component... Overrides>
    ActorConfig& withChild(std::string_view child, Overrides... overrides);

    /// @brief Adds a child actor by id, with C++ component values replacing its own (Actors::addChild()).
    /// @throws exceptions::ActorException When no actor has this id, or the child contains this actor.
    template <Component... Overrides>
    ActorConfig& withChild(ActorId child, Overrides... overrides);

  private:
    /// @return The id of the actor named `name`.
    /// @throws exceptions::ActorException When no actor has this name.
    [[nodiscard]] ActorId findChild(std::string_view name) const;

    World* _world;  ///< The world holding the actor.
    ActorId _id;    ///< The actor being filled.
};
}  // namespace rtype::ecs

#define RTYPE_ACTOR_CONFIG_INCLUDING_TPP
#include "ActorConfig.tpp"  // IWYU pragma: export
#undef RTYPE_ACTOR_CONFIG_INCLUDING_TPP
