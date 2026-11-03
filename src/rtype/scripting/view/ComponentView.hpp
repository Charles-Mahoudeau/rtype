/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentView
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "ViewScope.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"

namespace rtype::scripting::view {
/// @brief What a script holds to read and write one component of an entity: its bytes, in place.
///
/// @details Scripts index it by field name (`bullet.damage`), and the access reads or writes the bytes according to
/// the field's `FieldType`. It is only valid until the scope's generation moves on, so a script must not keep it.
struct ComponentView {
    const ecs::ComponentInfo* info{nullptr};  ///< The component the bytes belong to; outlives the view.
    std::span<std::byte> bytes;               ///< The component's value, `info->type.size` bytes.
    ViewScope* scope{nullptr};                ///< The scope that invalidates the view.
    std::uint64_t generation{0};              ///< The scope's generation when the view was created.
    bool readOnly{false};                     ///< Whether writes are refused (read terms of a query).
};
}  // namespace rtype::scripting::view
