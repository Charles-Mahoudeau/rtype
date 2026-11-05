/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RuntimeHandle
*/

#pragma once

#include "rtype/luau/Runtime.hpp"
#include "rtype/scripting/component/ComponentLayout.hpp"

namespace rtype::scripting::system {
/// @brief Lets the systems a script added reach its runtime, and tells them when it is gone.
///
/// @details Shared between the `ScriptHost`, which owns the runtime and clears the pointer when it is destroyed, and
/// the bodies of its systems, which may outlive it in the App and then skip their run.
struct RuntimeHandle {
    luau::Runtime* runtime{nullptr};  ///< The runtime of the host, or null once the host is destroyed.
    const component::ComponentLayouts* components{nullptr};  ///< The components of the host, valid with `runtime`.
};
}  // namespace rtype::scripting::system
