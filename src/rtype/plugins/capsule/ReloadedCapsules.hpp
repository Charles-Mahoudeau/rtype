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
/// @brief The capsules changed since the last frame, the "rtype.ReloadedCapsules" resource the CapsulePlugin inserts:
/// the CapsuleReloadable fills it between two frames with the files hot reload read again, a tool (the editor) with
/// the capsules it redefined (Capsules::define()), and the plugin's Render system "rtype.Capsule.HotReload" empties it,
/// reloading what was spawned from them.
struct ReloadedCapsules {
    std::vector<CapsuleId> capsules;  ///< Read again, not reloaded in the world yet.
};
RTYPE_ECS_RESOURCE(ReloadedCapsules, "rtype.ReloadedCapsules");
}  // namespace rtype::plugins::capsule
