/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Isolation
*/

#pragma once

#include <glm/ext/vector_float2.hpp>
#include <optional>
#include <string>

#include "editor/Export.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::ecs {
class World;
}  // namespace rtype::ecs

namespace rtype::editor::scene {

/// @brief Where a capsule opened alone is spawned: far from anything a game places, so nothing else is in view, and
/// near enough that a float still places it to a hundredth of a unit.
inline constexpr glm::vec2 kIsolatedOrigin{1.0e5F, 1.0e5F};

/// @brief A capsule shown alone, as a prefab is in its own mode: spawned under an anchor at kIsolatedOrigin, where the
/// editor's preview (IRenderer::setScenePreview()) looks at its root, while the game's camera goes on showing the game.
/// The capsule's own values do not change: the anchor holds the offset, so a respawn (an edit) keeps it isolated.
struct Isolation {
    plugins::capsule::CapsuleId capsule;  ///< The capsule shown.
    ecs::Entity anchor;                   ///< Its root's parent, at kIsolatedOrigin; despawned with it.
    ecs::Entity root;                     ///< The capsule's root.
};

/// @brief Spawns the capsule @p id alone, under its anchor.
/// @return The isolation, or why the capsule could not be spawned.
[[nodiscard]] RTYPE_EDITOR_API std::optional<Isolation> isolate(ecs::World& world, plugins::capsule::CapsuleId id,
                                                                std::string& error);

/// @return Where the capsule's root is in the world (its GlobalTransform): what the preview looks at.
[[nodiscard]] RTYPE_EDITOR_API glm::vec2 centerOf(const ecs::World& world, const Isolation& isolation);

/// @brief Despawns what isolate() spawned: the capsule goes with its anchor.
RTYPE_EDITOR_API void endIsolation(ecs::World& world, const Isolation& isolation);

}  // namespace rtype::editor::scene
