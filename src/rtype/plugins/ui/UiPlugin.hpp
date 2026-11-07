/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UiPlugin
*/

#pragma once

#include <span>
#include <string_view>

#include "plugins/ui/Export.hpp"
#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::plugins::ui {

/// @brief The user interface: entities with a UiNode, laid out by Yoga and drawn over everything else.
///
/// @details Registers the components and events of the UI, and inserts UiPointer and UiFocus. All its systems are on
/// the client. In PreUpdate, after the input: the pointer picks a node, the focus moves, and the widgets react, from
/// the layout of the last frame. In Render: "rtype.ui.layout" lays out the nodes into their UiComputed, then
/// "rtype.ui.draw" pushes them to the OverlayQueue, in view pixels, so it never touches the backend and needs no order
/// with the other plugins that draw. It requires the graphics, input and text plugins. See docs/Ui.md.
class RTYPE_PLUGIN_UI_API UiPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.Ui";                 ///< The plugin's name.
    static constexpr std::string_view kLayoutSystem = "rtype.ui.layout";  ///< Render: lays out the nodes.
    static constexpr std::string_view kDrawSystem = "rtype.ui.draw";      ///< Render: draws the nodes.

    UiPlugin() = default;
    ~UiPlugin() override = default;
    UiPlugin(const UiPlugin& other) = default;
    UiPlugin& operator=(const UiPlugin& other) = default;
    UiPlugin(UiPlugin&& other) noexcept = default;
    UiPlugin& operator=(UiPlugin&& other) noexcept = default;

    /// @return "rtype.Ui".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @return The GraphicsPlugin, the InputPlugin and the TextPlugin.
    [[nodiscard]] std::span<const std::string_view> getDependencies() const noexcept override;

    /// @brief Registers what the UI brings and adds its systems.
    void build(ecs::App& app) override;
};

/// @brief Registers `engine.ui` on @p registry: its library does it on every side, and so does building the plugin.
void addUiForms(ecs::PluginRegistry& registry);

}  // namespace rtype::plugins::ui
