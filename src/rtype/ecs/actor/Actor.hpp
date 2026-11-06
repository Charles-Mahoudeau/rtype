/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Actor
*/

#pragma once

#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/actor/ComponentValues.hpp"

namespace rtype::ecs {
/// @brief Dense index of an actor in its world's Actors, local to one world. Use the actor's name across worlds.
using ActorId = std::uint32_t;

/// @brief The id of no actor.
inline constexpr ActorId kInvalidActorId = std::numeric_limits<ActorId>::max();

/// @brief A child of an actor: another actor, spawned with it and parented to it, with values of its own.
struct ActorChild {
    ActorId actor{kInvalidActorId};  ///< The child's actor.
    ComponentValues overrides;       ///< Values replacing the child actor's (its Transform relative to the parent).
};

/// @brief A named template of an entity: the component values each entity spawned from it gets, copied, and the
/// child actors spawned with it.
/// @details Instances are copies: changing an actor only changes the entities spawned after. Its children are added
/// through Actors::addChild(), which refuses an actor that would contain itself.
class RTYPE_ECS_API Actor {
  public:
    /// @param id Its id in its Actors.
    /// @param name Its stable name, such as "game.Bydo".
    Actor(ActorId id, std::string name);
    ~Actor() = default;
    Actor(const Actor& other) = delete;
    Actor& operator=(const Actor& other) = delete;
    Actor(Actor&& other) noexcept = default;
    Actor& operator=(Actor&& other) noexcept = default;

    /// @return Its id in its Actors.
    [[nodiscard]] ActorId getId() const noexcept;

    /// @return Its stable name.
    [[nodiscard]] std::string_view getName() const noexcept;

    /// @return The values each spawned entity gets.
    [[nodiscard]] const ComponentValues& getValues() const noexcept;

    /// @return The values each spawned entity gets, to set or remove some.
    [[nodiscard]] ComponentValues& getValues() noexcept;

    /// @return The child actors spawned with it, in the order they were added.
    [[nodiscard]] std::span<const ActorChild> getChildren() const noexcept;

  private:
    friend class Actors;  ///< Adds the children, once it checked they make no cycle.

    ActorId _id;                        ///< Its id in its Actors.
    std::string _name;                  ///< Its stable name.
    ComponentValues _values;            ///< What each spawned entity gets.
    std::vector<ActorChild> _children;  ///< The child actors, in the order they were added.
};
}  // namespace rtype::ecs
