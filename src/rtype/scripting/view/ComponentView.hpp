/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentView
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

#include "ViewScope.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/scripting/Export.hpp"

namespace rtype::scripting::view {
/// @brief What a script holds to read and write one component of an entity, a resource or an event: its bytes, in
/// place.
///
/// @details Scripts index it by field name (`bullet.damage`), and the access reads or writes the bytes according to
/// the field's `FieldType`. It is only valid until the scope's generation moves on, so a script must not keep it.
struct RTYPE_SCRIPTING_API ComponentView {
    const ecs::TypeInfo* type{nullptr};  ///< The component, resource or event of the bytes; outlives the view.
    std::span<std::byte> bytes;          ///< The component's value, `type->size` bytes.
    std::shared_ptr<ViewScope>
        scope;                      ///< The scope that invalidates the view; shared so a retained view never dangles.
    std::uint64_t generation{0};    ///< The scope's generation when the view was created.
    bool readOnly{false};           ///< Whether writes are refused (read terms of a query).
    std::shared_ptr<void> storage;  ///< Keeps `bytes` alive when the view owns a copy of them; null otherwise.
};
}  // namespace rtype::scripting::view
