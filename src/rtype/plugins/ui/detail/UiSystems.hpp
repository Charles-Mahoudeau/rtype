/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UiSystems
*/

#pragma once

#include "plugins/graphics/OverlayQueue.hpp"
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/graphics/WindowEvents.hpp"  // IWYU pragma: keep
#include "plugins/input/Input.hpp"
#include "plugins/input/InputEvents.hpp"  // IWYU pragma: keep
#include "plugins/text/FontAssets.hpp"
#include "plugins/ui/UiComponents.hpp"
#include "plugins/ui/UiEvents.hpp"
#include "plugins/ui/UiNode.hpp"
#include "plugins/ui/UiResources.hpp"
#include "plugins/ui/UiWidgets.hpp"
#include "rtype/contract/platform/event/Event.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

// The systems of the UiPlugin, in the order of a frame.
namespace rtype::plugins::ui::detail {

/// @brief PreUpdate, client: finds what is under the pointer, from the last frame's layout, and follows its button.
void pickNodes(
    ecs::Res<graphics::ViewInfo> view, ecs::Res<input::Input> input, ecs::ResMut<UiPointer> pointer,
    ecs::EventReader<contract::platform::event::MouseButtonPressed> presses,
    ecs::EventReader<contract::platform::event::MouseButtonReleased> releases,
    ecs::Query<const UiComputed, const UiNode, ecs::Optional<const ecs::Parent>, ecs::Optional<const UiBackground>,
               ecs::Optional<const UiImage>, ecs::Optional<const UiInteraction>>
        nodes);

/// @brief PreUpdate, client: moves the focus with a click, Tab, the arrows or the directional pad, and activates it.
void navigateFocus(
    ecs::Res<UiPointer> pointer, ecs::ResMut<UiFocus> focus,
    ecs::EventReader<contract::platform::event::KeyPressed> keys,
    ecs::EventReader<contract::platform::event::GamepadButtonPressed> buttons,
    ecs::Query<const UiComputed, const UiInteraction, ecs::Optional<const UiSlider>, ecs::Optional<const UiTextInput>>
        nodes);

/// @brief PreUpdate, client: writes the state of every UiInteraction.
void applyInteraction(ecs::Res<UiPointer> pointer, ecs::Res<UiFocus> focus, ecs::Query<UiInteraction> nodes);

/// @brief PreUpdate, client: colours the buttons by state and sends UiClicked.
void updateButtons(ecs::EventWriter<UiClicked> clicks, ecs::Query<const UiInteraction, const UiButton> buttons,
                   ecs::Query<const UiInteraction, const UiButton, UiBackground> backgrounds,
                   ecs::Query<const UiInteraction, const UiButton, UiImage> images);

/// @brief PreUpdate, client: flips the activated toggles and sends UiValueChanged.
void updateToggles(ecs::EventWriter<UiValueChanged> changes, ecs::Query<const UiInteraction, UiToggle> toggles);

/// @brief PreUpdate, client: moves the sliders held by the pointer or adjusted by the focus, and sends UiValueChanged.
void updateSliders(ecs::Res<UiPointer> pointer, ecs::Res<UiFocus> focus, ecs::EventWriter<UiValueChanged> changes,
                   ecs::Query<const UiInteraction, const UiComputed, UiSlider> sliders);

/// @brief PreUpdate, client: scrolls the innermost UiScroll under the pointer, and keeps them all in their content.
void updateScrolls(ecs::Res<UiPointer> pointer, ecs::EventReader<contract::platform::event::MouseScrolled> wheel,
                   ecs::Query<const UiComputed, UiScroll> scrolls);

/// @brief PreUpdate, client: edits the focused UiTextInput.
void updateTextInputs(ecs::EventReader<contract::platform::event::TextEntered> typed,
                      ecs::EventReader<contract::platform::event::KeyPressed> keys,
                      ecs::EventWriter<UiTextChanged> changes, ecs::EventWriter<UiTextSubmitted> submits,
                      ecs::Query<const UiInteraction, UiTextInput, UiText> inputs);

/// @brief Render, client: lays out the nodes, and writes where they ended up in their UiComputed.
void layoutUi(ecs::Res<graphics::ViewInfo> view, ecs::Res<text::FontAssets> fonts, ecs::Commands& commands,
              ecs::Query<const UiNode, ecs::Optional<const ecs::Parent>, ecs::Optional<const ecs::Children>,
                         ecs::Optional<const UiText>, ecs::Optional<const UiScroll>>
                  nodes,
              ecs::Query<UiComputed> computed);

/// @brief Render, client: draws the laid out nodes, in order, over the frame.
void drawUi(ecs::ResAppend<graphics::OverlayQueue> overlay, ecs::Res<text::FontAssets> fonts,
            ecs::Res<graphics::TextureAssets> textures,
            ecs::Query<const UiComputed, ecs::Optional<const UiBackground>, ecs::Optional<const UiImage>,
                       ecs::Optional<const UiText>, ecs::Optional<const UiToggle>, ecs::Optional<const UiSlider>,
                       ecs::Optional<const UiTextInput>, ecs::Optional<const UiInteraction>>
                nodes);

}  // namespace rtype::plugins::ui::detail
