/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UiInput
*/

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/common.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "plugins/ui/detail/UiNavigation.hpp"
#include "plugins/ui/detail/UiSystems.hpp"
#include "plugins/ui/detail/UiText.hpp"
#include "rtype/contract/platform/input/Key.hpp"
#include "rtype/contract/renderer/view/CameraView.hpp"

namespace rtype::plugins::ui::detail {
namespace {
using contract::platform::input::GamepadButton;
using contract::platform::input::Key;
using contract::platform::input::MouseButton;

/// @return Whether @p point is in the rectangle @p rectangle: x, y, width, height.
[[nodiscard]] bool contains(const glm::vec4& rectangle, const glm::vec2 point) noexcept {
    return point.x >= rectangle.x && point.y >= rectangle.y && point.x < rectangle.x + rectangle.z &&
           point.y < rectangle.y + rectangle.w;
}

/// @return Whether @p point is on the node @p node, and in what clips it.
[[nodiscard]] bool isOver(const UiComputed& node, const glm::vec2 point) noexcept {
    return contains(glm::vec4{node.position.x, node.position.y, node.size.x, node.size.y}, point) &&
           contains(node.clip, point);
}

/// @return The pointer @p mouse, in window units, in view pixels.
[[nodiscard]] glm::vec2 toView(const graphics::ViewInfo& view, const glm::vec2 mouse) {
    if (view.framebuffer.x == 0 || view.framebuffer.y == 0 || view.window.x == 0 || view.window.y == 0) {
        return mouse;
    }
    const contract::renderer::CameraView shown{
        contract::renderer::CameraView::makeDefaultView(view.viewSize, view.framebuffer), view.viewSize,
        view.framebuffer};
    return shown.screenToWorld(mouse * glm::vec2{view.framebuffer} / glm::vec2{view.window});
}

/// @brief One node the pointer may be on, while picking.
struct Candidate {
    ecs::Entity entity;   ///< The node.
    ecs::Entity parent;   ///< Its parent, or null.
    std::uint32_t order;  ///< Its rank in the draw order.
    bool over;            ///< The pointer is on it.
    bool solid;           ///< It shows something, so it stops the pointer.
    bool interactive;     ///< It has an enabled UiInteraction.
};

/// @return The direction of a key or of a directional pad button, or nothing.
[[nodiscard]] std::optional<glm::vec2> directionOf(const Key key) noexcept {
    switch (key) {
        case Key::kLeft:
            return glm::vec2{-1.0F, 0.0F};
        case Key::kRight:
            return glm::vec2{1.0F, 0.0F};
        case Key::kUp:
            return glm::vec2{0.0F, -1.0F};
        case Key::kDown:
            return glm::vec2{0.0F, 1.0F};
        default:
            return std::nullopt;
    }
}

[[nodiscard]] std::optional<glm::vec2> directionOf(const GamepadButton button) noexcept {
    switch (button) {
        case GamepadButton::kDpadLeft:
            return glm::vec2{-1.0F, 0.0F};
        case GamepadButton::kDpadRight:
            return glm::vec2{1.0F, 0.0F};
        case GamepadButton::kDpadUp:
            return glm::vec2{0.0F, -1.0F};
        case GamepadButton::kDpadDown:
            return glm::vec2{0.0F, 1.0F};
        default:
            return std::nullopt;
    }
}

/// @brief One node the focus can reach.
struct Focusable {
    ecs::Entity entity;      ///< The node.
    glm::vec2 center{0.0F};  ///< Where it is.
    std::uint32_t order;     ///< Its rank in the draw order, which is also the order of Tab.
    bool slider;             ///< Left and right adjust it.
    bool typing;             ///< It is a UiTextInput.
};

/// @brief Moves the focus of @p focus in @p direction among @p items, or adjusts the focused slider.
void moveFocus(UiFocus& focus, const std::vector<Focusable>& items, const glm::vec2 direction) {
    const auto current = std::ranges::find(items, focus.focused, &Focusable::entity);
    if (current == items.end()) {
        if (!items.empty()) {
            focus.focused = items.front().entity;
        }
        return;
    }
    if (current->typing && direction.y == 0.0F) {
        return;  // The caret moves instead.
    }
    if (current->slider && direction.x != 0.0F) {
        focus.adjust = direction.x;
        return;
    }
    std::vector<glm::vec2> centers;
    centers.reserve(items.size());
    for (const Focusable& item : items) {
        centers.push_back(item.center);
    }
    if (const std::optional<std::size_t> next = findNext(centers, current->center, direction)) {
        focus.focused = items.at(*next).entity;
    }
}

/// @brief Moves the focus of @p focus to the next of @p items, or the previous one, wrapping around.
void cycleFocus(UiFocus& focus, const std::vector<Focusable>& items, const bool backwards) {
    if (items.empty()) {
        return;
    }
    const auto current = std::ranges::find(items, focus.focused, &Focusable::entity);
    const auto count = static_cast<std::ptrdiff_t>(items.size());
    const std::ptrdiff_t index = current == items.end() ? (backwards ? 0 : -1) : current - items.begin();
    focus.focused = items.at(static_cast<std::size_t>((index + (backwards ? -1 : 1) + count) % count)).entity;
}

/// @return The value of @p slider rounded to its step, and between its bounds.
[[nodiscard]] float snap(const UiSlider& slider, const float value) noexcept {
    const float low = std::min(slider.min, slider.max);
    const float high = std::max(slider.min, slider.max);
    const float stepped =
        slider.step > 0.0F ? slider.min + (std::round((value - slider.min) / slider.step) * slider.step) : value;
    return std::clamp(stepped, low, high);
}
}  // namespace

void pickNodes(
    ecs::Res<graphics::ViewInfo> view, ecs::Res<input::Input> input, ecs::ResMut<UiPointer> pointer,
    ecs::EventReader<contract::platform::event::MouseButtonPressed> presses,
    ecs::EventReader<contract::platform::event::MouseButtonReleased> releases,
    ecs::Query<const UiComputed, const UiNode, ecs::Optional<const ecs::Parent>, ecs::Optional<const UiBackground>,
               ecs::Optional<const UiImage>, ecs::Optional<const UiInteraction>>
        nodes) {
    pointer->position = toView(*view, input->getMousePosition());
    pointer->clicked = ecs::Entity{};

    std::vector<Candidate> candidates;
    std::unordered_map<ecs::Entity, std::size_t> byEntity;
    for (auto [entity, computed, node, parent, background, image, interaction] : nodes) {
        const bool visibleBackground =
            background != nullptr && (background->color.w > 0.0F || background->borderWidth > 0.0F);
        byEntity.emplace(entity, candidates.size());
        candidates.push_back(Candidate{.entity = entity,
                                       .parent = parent != nullptr ? parent->entity : ecs::Entity{},
                                       .order = computed.order,
                                       .over = !node.ignorePointer && isOver(computed, pointer->position),
                                       .solid = visibleBackground || image != nullptr,
                                       .interactive = interaction != nullptr && !interaction->disabled});
    }
    std::vector<const Candidate*> sorted;
    sorted.reserve(candidates.size());
    for (const Candidate& candidate : candidates) {
        sorted.push_back(&candidate);
    }
    std::ranges::sort(sorted, [](const Candidate* left, const Candidate* right) { return left->order > right->order; });

    pointer->overUi = false;
    pointer->hovered = ecs::Entity{};
    for (const Candidate* candidate : sorted) {
        if (!candidate->over || (!candidate->solid && !candidate->interactive)) {
            continue;
        }
        pointer->overUi = true;
        // What is drawn inside a button is the button: go up to the node that reacts.
        for (const Candidate* node = candidate; node != nullptr;) {
            if (node->interactive) {
                pointer->hovered = node->entity;
                break;
            }
            const auto parent = byEntity.find(node->parent);
            node = parent != byEntity.end() ? &candidates.at(parent->second) : nullptr;
        }
        break;
    }

    for (const auto& press : presses) {
        if (press.button == MouseButton::kLeft) {
            pointer->pressed = pointer->hovered;
        }
    }
    for (const auto& release : releases) {
        if (release.button == MouseButton::kLeft) {
            if (!pointer->pressed.isNull() && pointer->pressed == pointer->hovered) {
                pointer->clicked = pointer->pressed;
            }
            pointer->pressed = ecs::Entity{};
        }
    }
}

void navigateFocus(
    ecs::Res<UiPointer> pointer, ecs::ResMut<UiFocus> focus,
    ecs::EventReader<contract::platform::event::KeyPressed> keys,
    ecs::EventReader<contract::platform::event::GamepadButtonPressed> buttons,
    ecs::Query<const UiComputed, const UiInteraction, ecs::Optional<const UiSlider>, ecs::Optional<const UiTextInput>>
        nodes) {
    std::vector<Focusable> items;
    for (auto [entity, computed, interaction, slider, input] : nodes) {
        if (interaction.focusable && !interaction.disabled) {
            items.push_back(Focusable{.entity = entity,
                                      .center = computed.position + (computed.size * 0.5F),
                                      .order = computed.order,
                                      .slider = slider != nullptr,
                                      .typing = input != nullptr});
        }
    }
    std::ranges::sort(items, {}, &Focusable::order);
    focus->activated = false;
    focus->adjust = 0.0F;
    if (!pointer->pressed.isNull()) {
        focus->focused = std::ranges::find(items, pointer->pressed, &Focusable::entity) != items.end()
                             ? pointer->pressed
                             : ecs::Entity{};
    }
    const auto isTyping = [&items, &focus] {
        const auto current = std::ranges::find(items, focus->focused, &Focusable::entity);
        return current != items.end() && current->typing;
    };
    for (const auto& pressed : keys) {
        if (pressed.key == Key::kTab) {
            cycleFocus(*focus, items, pressed.mods.shift);
        } else if (pressed.key == Key::kEscape) {
            focus->focused = ecs::Entity{};
        } else if ((pressed.key == Key::kEnter || pressed.key == Key::kSpace) && !isTyping() && !pressed.repeat) {
            focus->activated = !focus->focused.isNull();
        } else if (const std::optional<glm::vec2> direction = directionOf(pressed.key)) {
            moveFocus(*focus, items, *direction);
        }
    }
    for (const auto& pressed : buttons) {
        if (pressed.button == GamepadButton::kSouth) {
            focus->activated = !focus->focused.isNull();
        } else if (pressed.button == GamepadButton::kEast) {
            focus->focused = ecs::Entity{};
        } else if (const std::optional<glm::vec2> direction = directionOf(pressed.button)) {
            moveFocus(*focus, items, *direction);
        }
    }
    focus->typing = isTyping();
}

void applyInteraction(ecs::Res<UiPointer> pointer, ecs::Res<UiFocus> focus, ecs::Query<UiInteraction> nodes) {
    for (auto [entity, interaction] : nodes) {
        const bool disabled = interaction.get().disabled;
        const bool focused = !disabled && entity == focus->focused;
        interaction->hovered = !disabled && entity == pointer->hovered;
        interaction->pressed = !disabled && entity == pointer->pressed;
        interaction->focused = focused;
        interaction->clicked = !disabled && (entity == pointer->clicked || (focused && focus->activated));
    }
}

namespace {
/// @return The colour of @p button in the state of @p interaction.
[[nodiscard]] contract::renderer::Color colorOf(const UiButton& button, const UiInteraction& interaction) noexcept {
    if (interaction.disabled) {
        return button.disabledColor;
    }
    if (interaction.pressed) {
        return button.pressedColor;
    }
    return interaction.hovered || interaction.focused ? button.hoveredColor : button.normalColor;
}
}  // namespace

void updateButtons(ecs::EventWriter<UiClicked> clicks, ecs::Query<const UiInteraction, const UiButton> buttons,
                   ecs::Query<const UiInteraction, const UiButton, UiBackground> backgrounds,
                   ecs::Query<const UiInteraction, const UiButton, UiImage> images) {
    for (auto [entity, interaction, button] : buttons) {
        if (interaction.clicked) {
            clicks.send(UiClicked{.entity = entity, .action = button.action});
        }
    }
    for (auto [entity, interaction, button, background] : backgrounds) {
        background->color = colorOf(button, interaction);
    }
    for (auto [entity, interaction, button, image] : images) {
        image->color = colorOf(button, interaction);
    }
}

void updateToggles(ecs::EventWriter<UiValueChanged> changes, ecs::Query<const UiInteraction, UiToggle> toggles) {
    for (auto [entity, interaction, toggle] : toggles) {
        if (interaction.clicked) {
            toggle->checked = !toggle.get().checked;
            changes.send(UiValueChanged{
                .entity = entity, .action = toggle.get().action, .value = toggle.get().checked ? 1.0F : 0.0F});
        }
    }
}

void updateSliders(ecs::Res<UiPointer> pointer, ecs::Res<UiFocus> focus, ecs::EventWriter<UiValueChanged> changes,
                   ecs::Query<const UiInteraction, const UiComputed, UiSlider> sliders) {
    constexpr float kDefaultKeyStep = 0.05F;
    for (auto [entity, interaction, computed, slider] : sliders) {
        const UiSlider& current = slider.get();
        float value = current.value;
        if (interaction.pressed && computed.size.x > 0.0F) {
            const float ratio = std::clamp((pointer->position.x - computed.position.x) / computed.size.x, 0.0F, 1.0F);
            value = current.min + (ratio * (current.max - current.min));
        } else if (interaction.focused && focus->adjust != 0.0F) {
            const float step = current.step > 0.0F ? current.step : (current.max - current.min) * kDefaultKeyStep;
            value += focus->adjust * step;
        }
        value = snap(current, value);
        if (value != current.value) {
            slider->value = value;
            changes.send(UiValueChanged{.entity = entity, .action = current.action, .value = value});
        }
    }
}

void updateScrolls(ecs::Res<UiPointer> pointer, ecs::EventReader<contract::platform::event::MouseScrolled> wheel,
                   ecs::Query<const UiComputed, UiScroll> scrolls) {
    glm::vec2 delta{0.0F};
    for (const auto& scrolled : wheel) {
        delta += scrolled.offset;
    }
    ecs::Entity target;
    std::uint32_t order = 0;
    for (auto [entity, computed, scroll] : scrolls) {
        if (delta != glm::vec2{0.0F} && isOver(computed, pointer->position) &&
            (target.isNull() || computed.order >= order)) {
            target = entity;
            order = computed.order;
        }
    }
    for (auto [entity, computed, scroll] : scrolls) {
        glm::vec2 offset = scroll.get().offset;
        if (entity == target) {
            offset -= delta * scroll.get().speed;
        }
        const glm::vec2 clamped =
            glm::clamp(offset, glm::vec2{0.0F}, glm::max(computed.contentSize - computed.size, glm::vec2{0.0F}));
        if (clamped != scroll.get().offset) {
            scroll->offset = clamped;
        }
    }
}

void updateTextInputs(ecs::EventReader<contract::platform::event::TextEntered> typed,
                      ecs::EventReader<contract::platform::event::KeyPressed> keys,
                      ecs::EventWriter<UiTextChanged> changes, ecs::EventWriter<UiTextSubmitted> submits,
                      ecs::Query<const UiInteraction, UiTextInput, UiText> inputs) {
    for (auto [entity, interaction, input, shown] : inputs) {
        if (!interaction.focused) {
            continue;
        }
        std::string text = shown.get().text;
        std::size_t caret = std::min<std::size_t>(input.get().caret, text.size());
        const std::uint32_t maxLength = input.get().maxLength;
        bool changed = false;
        for (const auto& entered : typed) {
            const std::string bytes = encodeUtf8(entered.codepoint);
            if (entered.codepoint >= U' ' && (maxLength == 0 || text.size() + bytes.size() <= maxLength)) {
                text.insert(caret, bytes);
                caret += bytes.size();
                changed = true;
            }
        }
        for (const auto& pressed : keys) {
            if (pressed.key == Key::kBackspace && caret > 0) {
                const std::size_t start = previousCharacter(text, caret);
                text.erase(start, caret - start);
                caret = start;
                changed = true;
            } else if (pressed.key == Key::kDelete && caret < text.size()) {
                text.erase(caret, nextCharacter(text, caret) - caret);
                changed = true;
            } else if (pressed.key == Key::kLeft) {
                caret = previousCharacter(text, caret);
            } else if (pressed.key == Key::kRight) {
                caret = nextCharacter(text, caret);
            } else if (pressed.key == Key::kHome) {
                caret = 0;
            } else if (pressed.key == Key::kEnd) {
                caret = text.size();
            } else if (pressed.key == Key::kEnter) {
                submits.send(UiTextSubmitted{.entity = entity, .action = input.get().action});
            }
        }
        if (caret != input.get().caret) {
            input->caret = static_cast<std::uint32_t>(caret);
        }
        if (changed) {
            shown->text = std::move(text);
            changes.send(UiTextChanged{.entity = entity, .action = input.get().action});
        }
    }
}
}  // namespace rtype::plugins::ui::detail
