/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputLayout
*/

#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include "rtype/replication/Export.hpp"
#include "rtype/replication/input/InputKind.hpp"

namespace rtype::replication::input {
/// @brief One declared input action.
struct InputAction {
    std::string name;                    ///< Unique name, 1 to 255 bytes.
    InputKind kind{InputKind::kButton};  ///< What the action carries.

    friend bool operator==(const InputAction& left, const InputAction& right) noexcept = default;
};

/// @brief The game's list of named input actions, the same on both sides (the manifest checks it).
///
/// @details Actions are kept ordered by name whatever the order of add() calls, so the position of an action is the
/// same on every machine. freeze() is called when the manifest is built: actions are declared at startup only.
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

  private:
    std::vector<InputAction> _actions;  ///< Ordered by name.
    bool _frozen{false};                ///< True after freeze().
};
}  // namespace rtype::replication::input
