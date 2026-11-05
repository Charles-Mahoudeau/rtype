/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentDeclaration
*/

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::scripting::component {
/// @brief One field of a component, as a script declares it.
struct FieldDeclaration {
    std::string name;                            ///< Field name, unique within the component.
    ecs::FieldType type{ecs::FieldType::kBool};  ///< Type of the field's value.
    std::vector<std::byte> defaultValue;         ///< Bytes of the default value; empty for zeros.
};

/// @brief A component as a script declares it: fields in declaration order, before any layout is computed.
struct ComponentDeclaration {
    std::string name;                                               ///< Stable, unique key: "game.Bullet".
    std::vector<FieldDeclaration> fields;                           ///< Fields, in the order the script wrote them.
    ecs::ReplicationMode replication{ecs::ReplicationMode::kNone};  ///< How the server sends the component.
    ecs::Presence presence{ecs::Presence::kBoth};                   ///< On which side of the network it exists.
};
}  // namespace rtype::scripting::component
