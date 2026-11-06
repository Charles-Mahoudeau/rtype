/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Actors
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/actor/Actor.hpp"
#include "rtype/ecs/actor/ComponentValues.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::ecs {
/// @brief The actors of one world, by id and by name, and the side they are spawned on.
/// @details Actors are added empty and filled through edit(); a child is added with addChild(), which refuses a
/// actor that would contain itself. Pointers returned by the lookups stay valid for the registry's lifetime (a
/// deque keeps their addresses). The run mode (set by the App) decides which components a spawn keeps: a client
/// leaves out server-only ones, a dedicated server client-only ones, standalone keeps them all.
/// @code
/// rtype::ecs::Actors& actors = world.getActors();
/// const rtype::ecs::ActorId turret = actors.add("game.Turret");
/// actors.edit(turret).getValues().set(world.getComponents(), Health{.value = 5});
/// @endcode
class RTYPE_ECS_API Actors {
  public:
    Actors() = default;
    ~Actors() = default;
    Actors(const Actors& other) = delete;
    Actors& operator=(const Actors& other) = delete;
    Actors(Actors&& other) noexcept = default;
    Actors& operator=(Actors&& other) noexcept = default;

    /// @return The side spawns happen on.
    [[nodiscard]] RunMode getRunMode() const noexcept;

    /// @brief Sets the side spawns happen on (the App does, from its run mode).
    void setRunMode(RunMode mode) noexcept;

    /// @return The number of actors; ids go from 0 to this count minus one.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @brief Adds an empty actor.
    /// @param name Its stable name, such as "game.Bydo".
    /// @return Its id.
    /// @throws exceptions::ActorException When the name is empty, already taken, or has the hash of another name.
    ActorId add(std::string name);

    /// @return The actor with this id, or nullptr.
    [[nodiscard]] const Actor* findById(ActorId id) const noexcept;

    /// @return The actor with this name, or nullptr.
    [[nodiscard]] const Actor* findByName(std::string_view name) const noexcept;

    /// @return The actor with this id, to change its values. Entities already spawned from it do not change.
    /// @throws exceptions::ActorException When no actor has this id.
    [[nodiscard]] Actor& edit(ActorId id);

    /// @brief Adds a child to an actor: spawned with it and parented to it, with `overrides` replacing its values.
    /// @throws exceptions::ActorException When either id is unknown, or the child contains the parent (or is it): a
    /// actor cannot contain itself.
    void addChild(ActorId parent, ActorId child, ComponentValues overrides = {});

    /// @return Whether a spawn on this registry's side keeps a component of this type: false for a server-only one on a
    /// client, and for a client-only one on a dedicated server.
    [[nodiscard]] bool keeps(const TypeInfo& type) const noexcept;

  private:
    /// @return Whether `target` is `root` or one of its children, at any depth.
    [[nodiscard]] bool contains(ActorId root, ActorId target) const noexcept;

    std::deque<Actor> _actors;                              ///< Actors by id; a deque keeps their addresses.
    std::unordered_map<std::uint64_t, ActorId> _idsByHash;  ///< Ids by name hash.
    RunMode _mode{RunMode::kStandalone};                    ///< The side spawns happen on.
};
}  // namespace rtype::ecs
