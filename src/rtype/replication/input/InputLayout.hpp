/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputLayout
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/replication/Export.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/input/InputKind.hpp"

namespace rtype::replication::input {
/// @brief One declared input action.
struct InputAction {
    std::string name;                    ///< Unique name, 1 to 255 bytes.
    InputKind kind{InputKind::kButton};  ///< What the action carries.

    friend bool operator==(const InputAction& left, const InputAction& right) noexcept = default;
};

/// @brief Where an action lives in a PlayerInput, found once with InputLayout::find() so that hot code does not look
/// names up every tick.
struct InputHandle {
    InputKind kind{InputKind::kButton};  ///< What the action carries.
    std::uint32_t index{0};              ///< The bit of a button, the first axis slot of an axis or a vector.

    friend bool operator==(const InputHandle& left, const InputHandle& right) noexcept = default;
};

/// @brief The game's list of named input actions, the same on both sides (the manifest checks it).
///
/// @details Actions are kept ordered by name whatever the order of add() calls, so the position of an action is the
/// same on every machine. freeze() is called when the manifest is built: actions are declared at startup only.
///
/// It decides where each action lives in a PlayerInput: button `i` is the `i`-th button action in name order (bit `i`
/// of `buttons`), and axis slots are given in that order, one for an axis and two for a vector. Gameplay code reads
/// actions by name and never deals with bits. It is an ECS resource, named "net.InputLayout".
class RTYPE_REPLICATION_API InputLayout {
  public:
    InputLayout() = default;
    ~InputLayout() = default;
    InputLayout(const InputLayout& other) = default;
    InputLayout& operator=(const InputLayout& other) = default;
    InputLayout(InputLayout&& other) noexcept = default;
    InputLayout& operator=(InputLayout&& other) noexcept = default;

    /// @return The actions ordered by name; an action's index is its position.
    [[nodiscard]] std::span<const InputAction> actions() const noexcept;

    /// @return True once freeze() was called.
    [[nodiscard]] bool isFrozen() const noexcept;

    /// @brief Declares an action.
    /// @param name The action's name, 1 to 255 bytes, not declared yet.
    /// @param kind What the action carries.
    /// @throws exceptions::InputLayoutException When the layout is frozen or the name is empty, too long or taken.
    void add(std::string name, InputKind kind);

    /// @brief Forbids any further add().
    void freeze() noexcept;

    /// @return The number of button actions (at most 32).
    [[nodiscard]] std::size_t buttonCount() const noexcept;

    /// @return The number of axis slots the actions use (at most component::kMaxInputAxes).
    [[nodiscard]] std::size_t axisSlotCount() const noexcept;

    /// @brief Finds where an action lives.
    /// @throws exceptions::InputLayoutException When no action has this name.
    [[nodiscard]] InputHandle find(std::string_view action) const;

    /// @brief Tells whether a button is held.
    /// @throws exceptions::InputLayoutException When the action is unknown or not a button.
    [[nodiscard]] bool isHeld(const component::PlayerInput& input, std::string_view action) const;
    [[nodiscard]] bool isHeld(const component::PlayerInput& input, InputHandle action) const;

    /// @brief Tells whether a button is held now and was not in `previousButtons`.
    /// @throws exceptions::InputLayoutException When the action is unknown or not a button.
    [[nodiscard]] bool isPressed(const component::PlayerInput& input, std::string_view action) const;
    [[nodiscard]] bool isPressed(const component::PlayerInput& input, InputHandle action) const;

    /// @brief Tells whether a button is not held now and was in `previousButtons`.
    /// @throws exceptions::InputLayoutException When the action is unknown or not a button.
    [[nodiscard]] bool isReleased(const component::PlayerInput& input, std::string_view action) const;
    [[nodiscard]] bool isReleased(const component::PlayerInput& input, InputHandle action) const;

    /// @brief Reads an axis.
    /// @throws exceptions::InputLayoutException When the action is unknown or not an axis.
    [[nodiscard]] float readAxis(const component::PlayerInput& input, std::string_view action) const;
    [[nodiscard]] float readAxis(const component::PlayerInput& input, InputHandle action) const;

    /// @brief Reads a two-dimensional vector.
    /// @throws exceptions::InputLayoutException When the action is unknown or not a vector.
    [[nodiscard]] std::array<float, 2> readVector(const component::PlayerInput& input, std::string_view action) const;
    [[nodiscard]] std::array<float, 2> readVector(const component::PlayerInput& input, InputHandle action) const;

    /// @brief Sets or clears a button bit.
    /// @throws exceptions::InputLayoutException When the action is unknown or not a button.
    void setButton(component::PlayerInput& input, std::string_view action, bool held) const;
    void setButton(component::PlayerInput& input, InputHandle action, bool held) const;

    /// @brief Writes an axis, clamped to [-1, 1]; NaN becomes 0.
    /// @throws exceptions::InputLayoutException When the action is unknown or not an axis.
    void setAxis(component::PlayerInput& input, std::string_view action, float value) const;
    void setAxis(component::PlayerInput& input, InputHandle action, float value) const;

    /// @brief Writes a vector, each value clamped to [-1, 1]; NaN becomes 0.
    /// @throws exceptions::InputLayoutException When the action is unknown or not a vector.
    void setVector(component::PlayerInput& input, std::string_view action, std::array<float, 2> value) const;
    void setVector(component::PlayerInput& input, InputHandle action, std::array<float, 2> value) const;

  private:
    /// @brief Gives every action its bit or slots again, after an insertion.
    void assignHandles();

    std::vector<InputAction> _actions;  ///< Ordered by name.
    std::vector<InputHandle> _handles;  ///< The handle of each action, in the same order.
    std::size_t _buttonCount{0};        ///< Number of button actions.
    std::size_t _axisSlotCount{0};      ///< Number of axis slots.
    bool _frozen{false};                ///< True after freeze().
};

RTYPE_ECS_RESOURCE(InputLayout, "net.InputLayout");
}  // namespace rtype::replication::input
