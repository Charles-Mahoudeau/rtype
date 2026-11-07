/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ToolFrame
*/

#pragma once

#include <functional>

#include "rtype/contract/platform/IPlatform.hpp"
#include "rtype/contract/renderer/IRenderer.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::graphics {

/// @brief The frame of a tool (an editor, a debug UI) that draws over the game, as the main-thread ECS resource
/// "rtype.ToolFrame".
///
/// @details The GraphicsPlugin's Render system "rtype.graphics.endFrame" calls it once per frame, after the overlay and
/// just before it ends the frame, with the platform and the renderer it owns: the tool draws meshes
/// (IRenderer::drawMesh()), asks for the scene to go to a texture (IRenderer::setSceneSize()) and reads what the frame
/// drew. It runs where the graphics plugin already owns the backend, so a tool needs no order against the plugins that
/// draw. It is not called while the window is minimized. Nothing to run, the default, costs nothing.
struct ToolFrame {
    std::function<void(contract::platform::IPlatform&, contract::renderer::IRenderer&)> draw;  ///< The tool's frame.
};
RTYPE_ECS_RESOURCE(ToolFrame, "rtype.ToolFrame");

}  // namespace rtype::plugins::graphics
