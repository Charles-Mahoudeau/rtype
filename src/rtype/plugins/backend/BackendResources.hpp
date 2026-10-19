/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** BackendResources
*/

#pragma once

#include <memory>

#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/config/Settings.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

// The started platform and renderer, as a main-thread resource: GLFW and Vulkan must be used from the main thread.
// The declaring macro must sit in the namespace of the type, so this reopens the engine's; engine-core itself stays
// free of the ECS.
namespace rtype::plugins::backend {
RTYPE_ECS_RESOURCE(Backend, "rtype.Backend");
}  // namespace rtype::plugins::backend

namespace rtype::plugins::backend {
/// @brief The colour each frame is cleared with.
struct ClearColor {
    plugins::backend::graphics::Color value{0.05F, 0.05F, 0.15F, 1.0F};  ///< RGBA, each in [0, 1].
};
RTYPE_ECS_RESOURCE(ClearColor, "rtype.ClearColor");

/// @brief What the Startup system needs to create the Backend: the registered platforms and renderers, and the
/// configuration naming the pair to start. Inserted by BackendPlugin::build(), which may not open a window itself.
struct BackendSetup {
    std::unique_ptr<plugins::backend::BackendRegistry> registry;  ///< The platforms and renderers by name.
    plugins::backend::config::Settings settings;                  ///< `platform`, `renderer`, `window`...
};
RTYPE_ECS_RESOURCE(BackendSetup, "rtype.BackendSetup");
}  // namespace rtype::plugins::backend
