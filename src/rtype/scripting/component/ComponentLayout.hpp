/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentLayout
*/

#pragma once

#include <cstddef>
#include <vector>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/type/Field.hpp"

namespace rtype::scripting::component {
/// @brief What registering a script component produced: its id and the layout every process computes the same way.
struct ComponentLayout {
    ecs::ComponentId id{ecs::kInvalidComponentId};  ///< Id in the world it was registered in.
    std::size_t size{0};                            ///< Bytes per value, a multiple of the alignment.
    std::size_t alignment{1};                       ///< Alignment of a value, the largest of its fields'.
    std::vector<ecs::FieldInfo> fields;             ///< Fields with their offsets, sorted by alignment then name.
    std::vector<std::byte> defaults;                ///< A default value: `size` bytes, fields set and padding zeroed.
};
}  // namespace rtype::scripting::component
