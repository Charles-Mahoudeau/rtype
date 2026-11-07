/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleInstance
*/

#pragma once

#include <cstdint>

#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::plugins::capsule {
/// @brief The root of a spawned capsule: which capsule, at which version, and whether another capsule spawned it.
/// @details Every spawned capsule's root has one, nested ones included. Despawning the root despawns the capsule (its
/// descendants go with it). A reload respawns the outermost capsule a nested root belongs to, so the outer node's
/// values stay on it.
struct CapsuleInstance {
    core::asset::AssetId capsule;  ///< The capsule (Capsule::getId()).
    std::uint32_t version{0};      ///< The capsule's version when it was spawned (Capsule::getVersion()).
    bool nested{false};            ///< Spawned as a node of another capsule, not by itself.
};
RTYPE_ECS_COMPONENT(CapsuleInstance, "rtype.CapsuleInstance", capsule, version, nested);
}  // namespace rtype::plugins::capsule
