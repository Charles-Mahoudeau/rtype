/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** LoadedCapsule
*/

#pragma once

#include "core/asset/AssetId.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::capsule {
/// @brief The capsule loadCapsule() loaded last, the "rtype.LoadedCapsule" resource: the level being played.
/// @details Loading another capsule despawns this one's root (and its descendants) first. Entities outside its tree,
/// the player or a manager, are not touched. Apart from the Capsules, so a load writes only this and spawns keep
/// reading the Capsules alongside.
struct LoadedCapsule {
    ecs::Entity root;              ///< The loaded capsule's root; the null entity before the first load.
    core::asset::AssetId capsule;  ///< The loaded capsule.
};
RTYPE_ECS_RESOURCE(LoadedCapsule, "rtype.LoadedCapsule");
}  // namespace rtype::plugins::capsule
