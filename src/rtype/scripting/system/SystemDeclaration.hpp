/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SystemDeclaration
*/

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "rtype/ecs/query/QueryTerm.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/luau/FunctionRef.hpp"

namespace rtype::scripting::system {
/// @brief One term of a script system's query: what it asks of a component, named as the script wrote it.
struct TermDeclaration {
    ecs::TermKind kind{ecs::TermKind::kRead};  ///< What the query asks of the component.
    std::string component;                     ///< Name of the component, "game.Shield".
};

/// @brief A resource a script system uses, named as the script wrote it.
struct ResourceDeclaration {
    std::string name;   ///< Name of the resource, "rtype.Time".
    bool write{false};  ///< Whether the system may write it (`resources.write`), false when it only reads it.
};

/// @brief An event type a script system uses, named as the script wrote it.
struct EventDeclaration {
    std::string name;  ///< Name of the event, "rtype.Collision".
    bool read{false};  ///< Whether the system reads the events (`events.read`).
    bool send{false};  ///< Whether the system sends events of this type (`events.send`).
};

/// @brief A system as a script declares it, before its components are looked up.
struct SystemDeclaration {
    std::string name;                            ///< Name of the system, unique: "game.ShieldRegen".
    ecs::Phase phase;                            ///< Phase the system runs in.
    std::optional<ecs::Side> side;               ///< Where it runs; empty for the ECS default (the server).
    std::vector<TermDeclaration> terms;          ///< Terms of its query, in the fixed order of readSystemDeclaration().
    std::vector<ResourceDeclaration> resources;  ///< Resources it uses, one entry per name.
    std::vector<EventDeclaration> events;        ///< Event types it uses, one entry per name.
    std::vector<std::string> calls;              ///< Engine calls `ctx.call` may make, in the order written.
    std::vector<std::string> after;              ///< Names of the systems it runs after.
    std::vector<std::string> before;             ///< Names of the systems it runs before.
    luau::FunctionRef run;                       ///< The `run` function of the declaration.
};
}  // namespace rtype::scripting::system
