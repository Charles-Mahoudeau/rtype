/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputCalls
*/

#include "plugins/input/InputCalls.hpp"

#include <array>
#include <format>
#include <string>
#include <string_view>

#include "plugins/input/Control.hpp"
#include "plugins/input/ControlNames.hpp"
#include "plugins/input/Input.hpp"
#include "plugins/input/InputAction.hpp"
#include "plugins/input/exceptions/InputExceptions.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

namespace rtype::plugins::input {
namespace {
/// @return The control named @p name.
/// @throws exceptions::InputException If no control has this name.
[[nodiscard]] Control requireControl(const std::string_view name) {
    const auto control = controlFromName(name);
    if (!control) {
        throw exceptions::InputException{std::format("Unknown control '{}'", name)};
    }
    return *control;
}

/// @return The action type named @p name.
/// @throws exceptions::InputException If no type has this name.
[[nodiscard]] ActionType requireActionType(const std::string_view name) {
    if (name == "button") {
        return ActionType::kButton;
    }
    if (name == "axis") {
        return ActionType::kAxis;
    }
    if (name == "vector2") {
        return ActionType::kVector2;
    }
    throw exceptions::InputException{
        std::format("Unknown action type '{}' (expected 'button', 'axis' or 'vector2')", name)};
}

void addAction(ecs::ResMut<Input> input, const AddActionArguments& arguments) {
    if (arguments.name.empty()) {
        throw exceptions::InputException{"An action needs a name"};
    }
    static_cast<void>(input->addAction(std::string{arguments.name.view()}, requireActionType(arguments.type)));
}

void bind(ecs::ResMut<Input> input, const BindArguments& arguments) {
    Control control = requireControl(arguments.control);
    control.scale *= arguments.scale;
    input->getAction(std::string{arguments.action.view()}).bind(control);
}

void bindAxis(ecs::ResMut<Input> input, const BindAxisArguments& arguments) {
    input->getAction(std::string{arguments.action.view()})
        .bindAxis(requireControl(arguments.negative), requireControl(arguments.positive));
}

void bindVector(ecs::ResMut<Input> input, const BindVectorArguments& arguments) {
    InputAction& action = input->getAction(std::string{arguments.action.view()});
    const bool fourControls =
        !arguments.up.empty() || !arguments.down.empty() || !arguments.left.empty() || !arguments.right.empty();
    const bool twoControls = !arguments.x.empty() || !arguments.y.empty();
    if (fourControls == twoControls) {
        throw exceptions::InputException{
            std::format("Action '{}': a vector is bound to 'up', 'down', 'left' and 'right', or to 'x' and 'y'",
                        arguments.action.view())};
    }
    if (fourControls) {
        action.bindVector(requireControl(arguments.up), requireControl(arguments.down), requireControl(arguments.left),
                          requireControl(arguments.right));
    } else {
        action.bindVector(requireControl(arguments.x), requireControl(arguments.y));
    }
}

ActionState readAction(ecs::Res<Input> input, const ReadActionArguments& arguments) {
    const InputAction& action = input->getAction(std::string{arguments.action.view()});
    return ActionState{
        .value = action.readVector(),
        .held = action.isHeld(),
        .pressed = action.isPressed(),
        .released = action.isReleased(),
    };
}
}  // namespace

void addInputForms(ecs::PluginRegistry& registry) {
    static constexpr auto kForms = std::to_array<char>({
#include "InputForms.luau.h"
    });
    // bin2c ends the bytes with a null terminator, which is not part of the source.
    registry.addScriptForms("addInputForms", std::string_view{kForms.data(), kForms.size() - 1});
}

void addInputCalls(ecs::App& app) {
    addInputForms(app.getPluginRegistry());
    app.addCall(std::string{kAddActionCall}, &addAction);
    app.addCall(std::string{kBindCall}, &bind);
    app.addCall(std::string{kBindAxisCall}, &bindAxis);
    app.addCall(std::string{kBindVectorCall}, &bindVector);
    app.addCall(std::string{kReadActionCall}, &readAction);
}

}  // namespace rtype::plugins::input
