/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ViewScope
*/

#pragma once

#include <cstdint>

namespace rtype::scripting::view {
/// @brief Tells whether the views a script system handed out are still valid.
///
/// @details One per script system, heap-allocated and owned by its `ScriptCallable`, so it keeps its address when the
/// callable is moved. A view stores the scope and the generation it was created with; the bridge increments the
/// generation when `run` returns, and every access to a view of an older generation raises a Luau error.
struct ViewScope {
    std::uint64_t generation{0};  ///< Incremented each time the views handed out so far become invalid.
};
}  // namespace rtype::scripting::view
