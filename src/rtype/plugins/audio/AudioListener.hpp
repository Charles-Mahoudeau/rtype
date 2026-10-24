/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioListener
*/

#pragma once

#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::plugins::audio {
/// @brief Where the player hears from: sounds are panned relative to the GlobalTransform of the entity with this
/// component (usually the camera). Without one, nothing is panned; with several, the first one found wins.
struct AudioListener {};
RTYPE_ECS_COMPONENT(AudioListener, "rtype.AudioListener");
}  // namespace rtype::plugins::audio
