/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ReloadedCapsules
*/

#pragma once

#include <vector>

#include "plugins/capsule/Capsule.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::capsule {
/// @brief The capsules read again since the last frame, the "rtype.ReloadedCapsules" resource the CapsulePlugin
/// inserts with hot reload: the CapsuleReloadable fills it between two frames, and the plugin's hot reload system
/// empties it, reloading what was spawned from them.
struct ReloadedCapsules {
    std::vector<CapsuleId> capsules;  ///< Read again, not reloaded in the world yet.
};
RTYPE_ECS_RESOURCE(ReloadedCapsules, "rtype.ReloadedCapsules");
}  // namespace rtype::plugins::capsule
