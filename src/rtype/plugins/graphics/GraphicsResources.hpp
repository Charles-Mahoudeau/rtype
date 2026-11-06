/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GraphicsResources
*/

#pragma once

#include <memory>

#include "core/config/Settings.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "rtype/contract/renderer/drawing/Color.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::graphics {
// The started platform and renderer, as a main-thread resource: GLFW and Vulkan must be used from the main thread.
RTYPE_ECS_RESOURCE(Backend, "rtype.Backend");

/// @brief The colour each frame is cleared with.
struct ClearColor {
    contract::renderer::Color value{0.05F, 0.05F, 0.15F, 1.0F};  ///< RGBA, each in [0, 1].
};
RTYPE_ECS_RESOURCE(ClearColor, "rtype.ClearColor");

/// @brief What the Startup system needs to create the Backend: the registered platforms and renderers, and the
/// configuration naming the pair to start. Inserted by GraphicsPlugin::build(), which may not open a window itself.
struct BackendSetup {
    std::unique_ptr<plugins::graphics::BackendRegistry> registry;  ///< The platforms and renderers by name.
    core::config::Settings settings;                               ///< `platform`, `renderer`, `window`...
};
RTYPE_ECS_RESOURCE(BackendSetup, "rtype.BackendSetup");
}  // namespace rtype::plugins::graphics
