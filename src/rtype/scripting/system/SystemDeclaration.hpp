/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SystemDeclaration
*/

#pragma once

#include <string>
#include <vector>

#include "rtype/ecs/query/QueryTerm.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/luau/FunctionRef.hpp"

namespace rtype::scripting::system {
/// @brief One term of a script system's query: what it asks of a component, named as the script wrote it.
struct TermDeclaration {
    ecs::TermKind kind{ecs::TermKind::kRead};  ///< What the query asks of the component.
    std::string component;                     ///< Name of the component, "game.Shield".
};

/// @brief A system as a script declares it, before its components are looked up.
struct SystemDeclaration {
    std::string name;                    ///< Name of the system, unique: "game.ShieldRegen".
    ecs::Phase phase;                    ///< Phase the system runs in.
    std::vector<TermDeclaration> terms;  ///< Terms of its query, in the fixed order of readSystemDeclaration().
    std::vector<std::string> after;      ///< Names of the systems it runs after.
    std::vector<std::string> before;     ///< Names of the systems it runs before.
    luau::FunctionRef run;               ///< The `run` function of the declaration.
};
}  // namespace rtype::scripting::system
