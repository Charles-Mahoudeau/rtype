/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleCalls
*/

#pragma once

#include <string_view>

#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "plugins/capsule/Export.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/call/CallTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::plugins::capsule {
inline constexpr std::string_view kSpawnCall = "rtype.capsule.spawn";    ///< spawnCapsule().
inline constexpr std::string_view kLoadCall = "rtype.capsule.load";      ///< loadCapsule(): switches levels.
inline constexpr std::string_view kReloadCall = "rtype.capsule.reload";  ///< reloadCapsule().

/// @brief The arguments of `rtype.capsule.spawn` and `rtype.capsule.load`: the capsule, by path from a script
/// ("actors/bydo.actor").
struct CapsuleArguments {
    core::asset::AssetId capsule;  ///< The capsule.
};
RTYPE_ECS_CALL_DATA(CapsuleArguments, "rtype.capsule.CapsuleArguments", capsule);

/// @brief The result of `rtype.capsule.spawn` and `rtype.capsule.load`: the root of the capsule spawned.
struct CapsuleRoot {
    ecs::Entity root;  ///< The root, reserved at once, alive when the system's commands are applied.
};
RTYPE_ECS_CALL_DATA(CapsuleRoot, "rtype.capsule.CapsuleRoot", root);

/// @brief The arguments of `rtype.capsule.reload`.
struct ReloadArguments {
    ecs::Entity root;  ///< The root of a spawned capsule.
};
RTYPE_ECS_CALL_DATA(ReloadArguments, "rtype.capsule.ReloadArguments", root);

/// @brief The result of `rtype.capsule.reload`.
struct ReloadResult {
    bool reloaded{false};  ///< False when `root` is not the root of a spawned capsule (or is gone).
};
RTYPE_ECS_CALL_DATA(ReloadResult, "rtype.capsule.ReloadResult", reloaded);

/// @brief Registers the capsule engine calls on @p app, for scripts: `rtype.capsule.spawn` (on `rtype.Capsules`),
/// `rtype.capsule.load` (also writes `rtype.LoadedCapsule`) and `rtype.capsule.reload` (also reads the
/// `rtype.CapsuleInstance` and `rtype.Children` components), each recording into the commands of the system that calls
/// it. They throw a CapsuleException naming a capsule that cannot be spawned. Called by the CapsulePlugin.
RTYPE_PLUGIN_CAPSULE_API void addCapsuleCalls(ecs::App& app);
}  // namespace rtype::plugins::capsule
