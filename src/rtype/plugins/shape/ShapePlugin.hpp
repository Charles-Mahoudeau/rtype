/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShapePlugin
*/

#pragma once

#include <span>
#include <string_view>

#include "plugins/shape/Export.hpp"
#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::plugins::shape {

/// @brief Draws the ShapeRenderer entities on the client: walls, panels, bars and markers made of plain shapes.
///
/// @details Registers the ShapeRenderer component, on every side so that a server can hold and replicate it, and adds
/// the client Render system "rtype.Shape.Draw", after the GraphicsPlugin's sprites and before the end of the frame: it
/// draws every entity with a GlobalTransform and a ShapeRenderer, sorted by layer, through the renderer. The renderer
/// draws shapes as overlays, so they come over the sprites. It requires the GraphicsPlugin and the TransformPlugin.
/// @code
/// app.addPlugin(rtype::plugins::transform::TransformPlugin{})
///     .addPlugin(rtype::plugins::input::InputPlugin{})
///     .addPlugin(rtype::plugins::graphics::GraphicsPlugin{std::move(registry), settings})
///     .addPlugin(rtype::plugins::shape::ShapePlugin{});
/// @endcode
class RTYPE_PLUGIN_SHAPE_API ShapePlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.Shape";             ///< The plugin's name.
    static constexpr std::string_view kDrawSystem = "rtype.Shape.Draw";  ///< Draws the shapes.

    ShapePlugin() = default;
    ~ShapePlugin() override = default;
    ShapePlugin(const ShapePlugin& other) = default;
    ShapePlugin& operator=(const ShapePlugin& other) = default;
    ShapePlugin(ShapePlugin&& other) noexcept = default;
    ShapePlugin& operator=(ShapePlugin&& other) noexcept = default;

    /// @return "rtype.Shape".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @return The GraphicsPlugin, which it draws through, and the TransformPlugin.
    [[nodiscard]] std::span<const std::string_view> getDependencies() const noexcept override;

    /// @brief Registers the ShapeRenderer component and adds the client Render system.
    void build(ecs::App& app) override;
};

}  // namespace rtype::plugins::shape
