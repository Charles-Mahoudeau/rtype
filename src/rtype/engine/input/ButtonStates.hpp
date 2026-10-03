/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ButtonStates
*/

#pragma once

#include <bitset>
#include <cstddef>

namespace rtype::engine::input {

/// @brief Held state of a family of buttons: keys, mouse buttons or gamepad buttons.
///
/// @details Events update the live state at any time. beginFrame() then takes the
/// snapshot that the frame reads: held now, or pressed at least once since the last
/// frame, so a tap shorter than a frame is never missed.
///
/// @tparam Button A button enum ending with a `kCount` enumerator (Key, MouseButton, GamepadButton).
template <typename Button>
class ButtonStates {
  public:
    ButtonStates() = default;
    ~ButtonStates() = default;
    ButtonStates(const ButtonStates& other) = default;
    ButtonStates& operator=(const ButtonStates& other) = default;
    ButtonStates(ButtonStates&& other) noexcept = default;
    ButtonStates& operator=(ButtonStates&& other) noexcept = default;

    /// @return True if the button was held, or tapped, when beginFrame() took the snapshot.
    [[nodiscard]] bool isDown(Button button) const;

    /// @brief Marks the button as held and as tapped for the next snapshot.
    void press(Button button);

    /// @brief Marks the button as no longer held. A tap stays counted until the next snapshot.
    void release(Button button);

    /// @brief Releases every button, e.g. when the window loses the focus.
    void releaseAll() noexcept;

    /// @brief Takes the snapshot read during this frame, then clears the taps.
    void beginFrame() noexcept;

  private:
    static constexpr std::size_t kCount = static_cast<std::size_t>(Button::kCount);

    /// @return The bit position of a button.
    [[nodiscard]] static constexpr std::size_t index(Button button) noexcept;

    /// @return True if the button fits in the bitsets (guards against out-of-range casts).
    [[nodiscard]] static constexpr bool isValid(Button button) noexcept;

    std::bitset<kCount> _down;    ///< Held right now.
    std::bitset<kCount> _tapped;  ///< Pressed at least once since the last beginFrame().
    std::bitset<kCount> _frame;   ///< Snapshot read during the current frame.
};

}  // namespace rtype::engine::input

#include "ButtonStates.tpp"
