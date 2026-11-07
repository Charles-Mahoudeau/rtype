/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UiPlugin
*/

#include "plugins/ui/UiPlugin.hpp"

#include <array>
#include <span>
#include <string>
#include <string_view>

#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/text/TextPlugin.hpp"
#include "plugins/ui/UiComponents.hpp"
#include "plugins/ui/UiEvents.hpp"
#include "plugins/ui/UiNode.hpp"
#include "plugins/ui/UiResources.hpp"
#include "plugins/ui/UiWidgets.hpp"
#include "plugins/ui/detail/UiSystems.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/Side.hpp"

namespace rtype::plugins::ui {

void addUiForms(ecs::PluginRegistry& registry) {
    static constexpr auto kForms = std::to_array<char>({
#include "UiForms.luau.h"
    });
    // bin2c ends the bytes with a null terminator, which is not part of the source.
    registry.addScriptForms("addUiForms", std::string_view{kForms.data(), kForms.size() - 1});
}

std::string_view UiPlugin::getName() const noexcept { return kName; }

std::span<const std::string_view> UiPlugin::getDependencies() const noexcept {
    static constexpr std::array<std::string_view, 3> kDependencies{graphics::GraphicsPlugin::kName,
                                                                   input::InputPlugin::kName, text::TextPlugin::kName};
    return kDependencies;
}

void UiPlugin::build(ecs::App& app) {
    ecs::Components& components = app.getWorld().getComponents();
    static_cast<void>(components.registerType<UiNode>());
    static_cast<void>(components.registerType<UiComputed>());
    static_cast<void>(components.registerType<UiBackground>());
    static_cast<void>(components.registerType<UiImage>());
    static_cast<void>(components.registerType<UiText>());
    static_cast<void>(components.registerType<UiInteraction>());
    static_cast<void>(components.registerType<UiButton>());
    static_cast<void>(components.registerType<UiToggle>());
    static_cast<void>(components.registerType<UiSlider>());
    static_cast<void>(components.registerType<UiScroll>());
    static_cast<void>(components.registerType<UiTextInput>());
    ecs::Events& events = app.getWorld().getEvents();
    static_cast<void>(events.registerType<UiClicked>());
    static_cast<void>(events.registerType<UiValueChanged>());
    static_cast<void>(events.registerType<UiTextChanged>());
    static_cast<void>(events.registerType<UiTextSubmitted>());
    ecs::Resources& resources = app.getWorld().getResources();
    resources.insertIfAbsent<UiPointer>();
    resources.insertIfAbsent<UiFocus>();
    addUiForms(app.getPluginRegistry());

    const auto preUpdate = [&app](const std::string_view name, auto system, const std::string_view after) {
        app.addSystem(ecs::Phase::kPreUpdate, std::string{name}, system)
            .after(std::string{after})
            .setSide(ecs::Side::kClient);
    };
    preUpdate("rtype.ui.pick", &detail::pickNodes, input::InputPlugin::kUpdateSystem);
    preUpdate("rtype.ui.navigate", &detail::navigateFocus, "rtype.ui.pick");
    preUpdate("rtype.ui.interact", &detail::applyInteraction, "rtype.ui.navigate");
    preUpdate("rtype.ui.buttons", &detail::updateButtons, "rtype.ui.interact");
    preUpdate("rtype.ui.toggles", &detail::updateToggles, "rtype.ui.buttons");
    preUpdate("rtype.ui.sliders", &detail::updateSliders, "rtype.ui.toggles");
    preUpdate("rtype.ui.scrolls", &detail::updateScrolls, "rtype.ui.sliders");
    preUpdate("rtype.ui.inputs", &detail::updateTextInputs, "rtype.ui.scrolls");
    app.addSystem(ecs::Phase::kRender, std::string{kLayoutSystem}, &detail::layoutUi)
        .after(std::string{graphics::GraphicsPlugin::kCameraSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kDrawSystem}, &detail::drawUi)
        .after(std::string{kLayoutSystem})
        .before(std::string{graphics::GraphicsPlugin::kEndFrameSystem})
        .setSide(ecs::Side::kClient);
}

}  // namespace rtype::plugins::ui
