/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GuiInput
*/

#include "GuiInput.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <limits>
#include <variant>

#include "GameViewport.hpp"
#include "rtype/contract/platform/event/Event.hpp"
#include "rtype/contract/platform/input/Key.hpp"

namespace rtype::editor::gui {

namespace {
namespace platform = contract::platform;
using Key = platform::input::Key;
using MouseButton = platform::input::MouseButton;

/// @return The key ImGui knows by this name, or ImGuiKey_None for a key it has no use for.
ImGuiKey toImGuiKey(Key key) {
    const auto offset = [key](Key first) { return static_cast<int>(key) - static_cast<int>(first); };
    if (key >= Key::kA && key <= Key::kZ) {
        return static_cast<ImGuiKey>(ImGuiKey_A + offset(Key::kA));
    }
    if (key >= Key::kNum0 && key <= Key::kNum9) {
        return static_cast<ImGuiKey>(ImGuiKey_0 + offset(Key::kNum0));
    }
    if (key >= Key::kF1 && key <= Key::kF12) {
        return static_cast<ImGuiKey>(ImGuiKey_F1 + offset(Key::kF1));
    }
    if (key >= Key::kKeypad0 && key <= Key::kKeypad9) {
        return static_cast<ImGuiKey>(ImGuiKey_Keypad0 + offset(Key::kKeypad0));
    }
    switch (key) {
        case Key::kSpace:
            return ImGuiKey_Space;
        case Key::kApostrophe:
            return ImGuiKey_Apostrophe;
        case Key::kComma:
            return ImGuiKey_Comma;
        case Key::kMinus:
            return ImGuiKey_Minus;
        case Key::kPeriod:
            return ImGuiKey_Period;
        case Key::kSlash:
            return ImGuiKey_Slash;
        case Key::kSemicolon:
            return ImGuiKey_Semicolon;
        case Key::kEqual:
            return ImGuiKey_Equal;
        case Key::kLeftBracket:
            return ImGuiKey_LeftBracket;
        case Key::kBackslash:
            return ImGuiKey_Backslash;
        case Key::kRightBracket:
            return ImGuiKey_RightBracket;
        case Key::kGrave:
            return ImGuiKey_GraveAccent;
        case Key::kEscape:
            return ImGuiKey_Escape;
        case Key::kEnter:
            return ImGuiKey_Enter;
        case Key::kTab:
            return ImGuiKey_Tab;
        case Key::kBackspace:
            return ImGuiKey_Backspace;
        case Key::kInsert:
            return ImGuiKey_Insert;
        case Key::kDelete:
            return ImGuiKey_Delete;
        case Key::kRight:
            return ImGuiKey_RightArrow;
        case Key::kLeft:
            return ImGuiKey_LeftArrow;
        case Key::kDown:
            return ImGuiKey_DownArrow;
        case Key::kUp:
            return ImGuiKey_UpArrow;
        case Key::kPageUp:
            return ImGuiKey_PageUp;
        case Key::kPageDown:
            return ImGuiKey_PageDown;
        case Key::kHome:
            return ImGuiKey_Home;
        case Key::kEnd:
            return ImGuiKey_End;
        case Key::kCapsLock:
            return ImGuiKey_CapsLock;
        case Key::kScrollLock:
            return ImGuiKey_ScrollLock;
        case Key::kNumLock:
            return ImGuiKey_NumLock;
        case Key::kPrintScreen:
            return ImGuiKey_PrintScreen;
        case Key::kPause:
            return ImGuiKey_Pause;
        case Key::kKeypadDecimal:
            return ImGuiKey_KeypadDecimal;
        case Key::kKeypadDivide:
            return ImGuiKey_KeypadDivide;
        case Key::kKeypadMultiply:
            return ImGuiKey_KeypadMultiply;
        case Key::kKeypadSubtract:
            return ImGuiKey_KeypadSubtract;
        case Key::kKeypadAdd:
            return ImGuiKey_KeypadAdd;
        case Key::kKeypadEnter:
            return ImGuiKey_KeypadEnter;
        case Key::kKeypadEqual:
            return ImGuiKey_KeypadEqual;
        case Key::kLeftShift:
            return ImGuiKey_LeftShift;
        case Key::kLeftControl:
            return ImGuiKey_LeftCtrl;
        case Key::kLeftAlt:
            return ImGuiKey_LeftAlt;
        case Key::kLeftSuper:
            return ImGuiKey_LeftSuper;
        case Key::kRightShift:
            return ImGuiKey_RightShift;
        case Key::kRightControl:
            return ImGuiKey_RightCtrl;
        case Key::kRightAlt:
            return ImGuiKey_RightAlt;
        case Key::kRightSuper:
            return ImGuiKey_RightSuper;
        case Key::kMenu:
            return ImGuiKey_Menu;
        default:
            return ImGuiKey_None;
    }
}

/// @return The index ImGui numbers a mouse button by, or -1 for a button it does not have.
int toImGuiButton(MouseButton button) {
    switch (button) {
        case MouseButton::kLeft:
            return ImGuiMouseButton_Left;
        case MouseButton::kRight:
            return ImGuiMouseButton_Right;
        case MouseButton::kMiddle:
            return ImGuiMouseButton_Middle;
        default:
            return -1;
    }
}

/// @brief Tells ImGui which modifiers are held, from the ones the event carries.
void feedModifiers(ImGuiIO& io, const platform::input::Mods& mods) {
    io.AddKeyEvent(ImGuiMod_Ctrl, mods.control);
    io.AddKeyEvent(ImGuiMod_Shift, mods.shift);
    io.AddKeyEvent(ImGuiMod_Alt, mods.alt);
    io.AddKeyEvent(ImGuiMod_Super, mods.super);
}

/// @brief Moves ImGui's mouse, and @p moved into the scene's pixels when it goes to the game.
/// @return Whether the game gets the move: over the game, or while a button pressed over it is held.
bool filterMouseMoved(ImGuiIO& io, platform::event::MouseMoved& moved, const GameViewport& viewport,
                      std::uint32_t gameButtons) {
    io.AddMousePosEvent(moved.position.x, moved.position.y);
    if (!viewport.hovered && gameButtons == 0) {
        return false;
    }
    // Window units to the scene's pixels: the game sees the scene as a window of its own.
    const glm::vec2 scale = viewport.size.x > 0.0F && viewport.size.y > 0.0F
                                ? glm::vec2{viewport.sceneSize} / viewport.size
                                : glm::vec2{1.0F};
    moved.position = (moved.position - viewport.position) * scale;
    moved.delta *= scale;
    return true;
}

/// @brief Tells ImGui that @p button went @p down, and keeps in @p gameButtons the ones pressed over the game.
/// @return Whether the game gets it: over the game, or the release of a button pressed over it.
bool filterMouseButton(ImGuiIO& io, int button, bool down, bool hovered, std::uint32_t& gameButtons) {
    if (button < 0) {
        return hovered;
    }
    io.AddMouseButtonEvent(button, down);
    const std::uint32_t bit = 1U << button;
    if (down) {
        if (hovered) {
            gameButtons |= bit;
        }
        return hovered;
    }
    const bool heldByGame = (gameButtons & bit) != 0;
    gameButtons &= ~bit;
    return heldByGame || hovered;
}
}  // namespace

GuiInput::GuiInput(ImGuiContext* context) noexcept : _context{context} {}

bool GuiInput::filter(platform::Event& event) {
    ImGui::SetCurrentContext(_context);
    ImGuiIO& io = ImGui::GetIO();
    const bool keyboardForGame = _viewport.focused && !io.WantTextInput;

    if (auto* moved = std::get_if<platform::event::MouseMoved>(&event)) {
        return filterMouseMoved(io, *moved, _viewport, _gameButtons);
    }
    if (const auto* pressed = std::get_if<platform::event::MouseButtonPressed>(&event)) {
        const bool forGame =
            filterMouseButton(io, toImGuiButton(pressed->button), true, _viewport.hovered, _gameButtons);
        feedModifiers(io, pressed->mods);
        return forGame;
    }
    if (const auto* released = std::get_if<platform::event::MouseButtonReleased>(&event)) {
        return filterMouseButton(io, toImGuiButton(released->button), false, _viewport.hovered, _gameButtons);
    }
    if (const auto* scrolled = std::get_if<platform::event::MouseScrolled>(&event)) {
        io.AddMouseWheelEvent(scrolled->offset.x, scrolled->offset.y);
        return _viewport.hovered;
    }
    if (const auto* pressed = std::get_if<platform::event::KeyPressed>(&event)) {
        feedModifiers(io, pressed->mods);
        io.AddKeyEvent(toImGuiKey(pressed->key), true);
        return keyboardForGame;
    }
    if (const auto* released = std::get_if<platform::event::KeyReleased>(&event)) {
        feedModifiers(io, released->mods);
        io.AddKeyEvent(toImGuiKey(released->key), false);
        // A key released after the focus left the game must still reach it, or the game keeps it down.
        return true;
    }
    if (const auto* text = std::get_if<platform::event::TextEntered>(&event)) {
        io.AddInputCharacter(static_cast<unsigned int>(text->codepoint));
        return keyboardForGame;
    }
    if (const auto* focus = std::get_if<platform::event::FocusChanged>(&event)) {
        io.AddFocusEvent(focus->focused);
        return true;
    }
    if (std::holds_alternative<platform::event::Resized>(event)) {
        if (_viewport.sceneSize.x == 0 || _viewport.sceneSize.y == 0) {
            return false;
        }
        constexpr auto kMax = std::numeric_limits<std::uint16_t>::max();
        event = platform::event::Resized{
            .width = static_cast<std::uint16_t>(std::min<std::uint32_t>(_viewport.sceneSize.x, kMax)),
            .height = static_cast<std::uint16_t>(std::min<std::uint32_t>(_viewport.sceneSize.y, kMax))};
        return true;
    }
    return true;
}

}  // namespace rtype::editor::gui
