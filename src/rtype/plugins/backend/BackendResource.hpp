/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** BackendResource
*/

#pragma once

#include "plugins/backend/BackendRegistry.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

// The started platform and renderer as the main-thread resource "rtype.Backend" (GLFW and Vulkan must be used from the
// main thread): the BackendPlugin creates it, and the plugins that draw (shader, particle, the physics debug view) use
// its renderer. Part of the contract, so they reach the renderer without knowing the backend plugin. Apart from
// BackendRegistry.hpp, which the platforms and renderers include and which stays free of the ECS. The declaring macro
// must sit in the namespace of the type, so this reopens the engine's.
namespace rtype::plugins::backend {
RTYPE_ECS_RESOURCE(Backend, "rtype.Backend");
}  // namespace rtype::plugins::backend
