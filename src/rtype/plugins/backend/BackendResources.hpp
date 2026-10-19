/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** BackendResources
*/

#pragma once

#include <memory>

#include "engine/backend/BackendRegistry.hpp"
#include "engine/config/Settings.hpp"
#include "engine/graphics/Color.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

// The started platform and renderer, as a main-thread resource: GLFW and Vulkan must be used from the main thread.
// The declaring macro must sit in the namespace of the type, so this reopens the engine's; engine-core itself stays
// free of the ECS.
namespace rtype::engine::backend {
RTYPE_ECS_RESOURCE(Backend, "rtype.Backend");
}  // namespace rtype::engine::backend

namespace rtype::plugins::backend {
/// @brief The colour each frame is cleared with.
struct ClearColor {
    engine::graphics::Color value{0.05F, 0.05F, 0.15F, 1.0F};  ///< RGBA, each in [0, 1].
};
RTYPE_ECS_RESOURCE(ClearColor, "rtype.ClearColor");

/// @brief What the Startup system needs to create the Backend: the registered platforms and renderers, and the
/// configuration naming the pair to start. Inserted by BackendPlugin::build(), which may not open a window itself.
struct BackendSetup {
    std::unique_ptr<engine::backend::BackendRegistry> registry;  ///< The platforms and renderers by name.
    engine::config::Settings settings;                           ///< `platform`, `renderer`, `window`...
};
RTYPE_ECS_RESOURCE(BackendSetup, "rtype.BackendSetup");
}  // namespace rtype::plugins::backend
