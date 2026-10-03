/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ButtonStates
*/

#include <cstddef>

namespace rtype::engine::input {

template <typename Button>
bool ButtonStates<Button>::isDown(Button button) const {
    return isValid(button) && _frame.test(index(button));
}

template <typename Button>
void ButtonStates<Button>::press(Button button) {
    if (isValid(button)) {
        _down.set(index(button));
        _tapped.set(index(button));
    }
}

template <typename Button>
void ButtonStates<Button>::release(Button button) {
    if (isValid(button)) {
        _down.reset(index(button));
    }
}

template <typename Button>
void ButtonStates<Button>::releaseAll() noexcept {
    _down.reset();
}

template <typename Button>
void ButtonStates<Button>::beginFrame() noexcept {
    _frame = _down | _tapped;
    _tapped.reset();
}

template <typename Button>
constexpr std::size_t ButtonStates<Button>::index(Button button) noexcept {
    return static_cast<std::size_t>(button);
}

template <typename Button>
constexpr bool ButtonStates<Button>::isValid(Button button) noexcept {
    return index(button) < kCount;
}

}  // namespace rtype::engine::input
