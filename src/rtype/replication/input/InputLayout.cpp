/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputLayout
*/

#include "rtype/replication/input/InputLayout.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <functional>
#include <span>
#include <string>
#include <utility>

#include "rtype/replication/exceptions/ReplicationExceptions.hpp"
#include "rtype/replication/input/InputKind.hpp"

namespace rtype::replication::input {
namespace {
/// @brief The longest name the wire can carry.
constexpr std::size_t kMaxNameLength = 255;
}  // namespace

std::span<const InputAction> InputLayout::actions() const noexcept { return _actions; }

bool InputLayout::isFrozen() const noexcept { return _frozen; }

void InputLayout::add(std::string name, const InputKind kind) {
    if (_frozen) {
        throw exceptions::InputLayoutException{
            std::format("Cannot add the input action '{}': the layout is frozen, declare actions at startup", name)};
    }
    if (name.empty() || name.size() > kMaxNameLength) {
        throw exceptions::InputLayoutException{
            std::format("Invalid input action name of {} bytes: it must have 1 to {}", name.size(), kMaxNameLength)};
    }
    const auto position = std::ranges::lower_bound(_actions, name, std::ranges::less{}, &InputAction::name);
    if (position != _actions.end() && position->name == name) {
        throw exceptions::InputLayoutException{std::format("The input action '{}' is already declared", name)};
    }
    _actions.insert(position, InputAction{.name = std::move(name), .kind = kind});
}

void InputLayout::freeze() noexcept { _frozen = true; }
}  // namespace rtype::replication::input
