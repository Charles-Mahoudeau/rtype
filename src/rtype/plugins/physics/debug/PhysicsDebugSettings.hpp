/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsDebugSettings
*/

#pragma once

#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/input/Key.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::physics::debug {

/// @brief How the PhysicsDebugPlugin draws: the global switches next to the per-entity PhysicsDebug component.
/// @details Inserted with its defaults by the plugin if the App has none; insert one before or after the plugin to
/// change them.
/// @code
/// app.insertResource(rtype::plugins::physics::debug::PhysicsDebugSettings{.drawAll = true});  // Every collider, F3
/// toggles.
/// @endcode
struct PhysicsDebugSettings {
    bool enabled = true;                                       ///< Draws nothing when false; the toggle key flips it.
    bool drawAll = false;                                      ///< Draws every collider and speed, PhysicsDebug or not.
    backend::input::Key toggleKey = backend::input::Key::kF3;  ///< Flips enabled; kUnknown: no key.
    backend::graphics::Color color{0.2F, 1.0F, 0.4F, 1.0F};    ///< For entities drawn by drawAll alone.
    float outlineThickness = 1.5F;                             ///< Of outlines and arrows, in world units.
    float velocityScale = 0.1F;  ///< Seconds of travel an arrow shows: a speed of 300 draws a 30 units long arrow.
};
RTYPE_ECS_RESOURCE(PhysicsDebugSettings, "rtype.PhysicsDebugSettings");

}  // namespace rtype::plugins::physics::debug
