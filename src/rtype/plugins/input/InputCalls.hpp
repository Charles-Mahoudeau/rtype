/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputCalls
*/

#pragma once

#include <glm/ext/vector_float2.hpp>
#include <string_view>

#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/call/CallTraits.hpp"
#include "rtype/ecs/type/String.hpp"

namespace rtype::plugins::input {

inline constexpr std::string_view kAddActionCall = "rtype.input.addAction";    ///< Input::addAction().
inline constexpr std::string_view kBindCall = "rtype.input.bind";              ///< InputAction::bind().
inline constexpr std::string_view kBindAxisCall = "rtype.input.bindAxis";      ///< InputAction::bindAxis().
inline constexpr std::string_view kBindVectorCall = "rtype.input.bindVector";  ///< InputAction::bindVector().
inline constexpr std::string_view kReadActionCall = "rtype.input.read";        ///< The state of an action.

/// @brief The arguments of `rtype.input.addAction`: creates (or replaces) an action.
struct AddActionArguments {
    ecs::String name;            ///< The action's name, as gameplay reads it ("move", "shoot").
    ecs::String type{"button"};  ///< "button", "axis" or "vector2".
};
RTYPE_ECS_CALL_DATA(AddActionArguments, "rtype.input.AddActionArguments", name, type);

/// @brief The arguments of `rtype.input.bind`: binds one control to an action.
struct BindArguments {
    ecs::String action;   ///< The action.
    ecs::String control;  ///< The control, named as controlFromName() reads it ("Space", "MouseLeft", "GamepadLeftX").
    float scale{1.0F};    ///< Multiplies the control's raw value (sensitivity, inversion).
};
RTYPE_ECS_CALL_DATA(BindArguments, "rtype.input.BindArguments", action, control, scale);

/// @brief The arguments of `rtype.input.bindAxis`: binds a 1D composite (-1 for `negative`, +1 for `positive`).
struct BindAxisArguments {
    ecs::String action;    ///< The action.
    ecs::String negative;  ///< The control that gives -1.
    ecs::String positive;  ///< The control that gives +1.
};
RTYPE_ECS_CALL_DATA(BindAxisArguments, "rtype.input.BindAxisArguments", action, negative, positive);

/// @brief The arguments of `rtype.input.bindVector`: binds a 2D vector, either from four controls (`up`, `down`,
/// `left`, `right`: WASD, the arrows, the d-pad) or from two analog ones (`x`, `y`: a stick).
struct BindVectorArguments {
    ecs::String action;  ///< The action.
    ecs::String up;      ///< Four controls: up.
    ecs::String down;    ///< Four controls: down.
    ecs::String left;    ///< Four controls: left.
    ecs::String right;   ///< Four controls: right.
    ecs::String x;       ///< Two analog controls: the horizontal one.
    ecs::String y;       ///< Two analog controls: the vertical one.
};
RTYPE_ECS_CALL_DATA(BindVectorArguments, "rtype.input.BindVectorArguments", action, up, down, left, right, x, y);

/// @brief The arguments of `rtype.input.read`.
struct ReadActionArguments {
    ecs::String action;  ///< The action.
};
RTYPE_ECS_CALL_DATA(ReadActionArguments, "rtype.input.ReadActionArguments", action);

/// @brief The result of `rtype.input.read`: an action's state this frame.
struct ActionState {
    glm::vec2 value{0.0F};  ///< Its value: the X alone for a button or an axis.
    bool held{false};       ///< Down this frame.
    bool pressed{false};    ///< Went down this frame.
    bool released{false};   ///< Went up this frame.
};
RTYPE_ECS_CALL_DATA(ActionState, "rtype.input.ActionState", value, held, pressed, released);

/// @brief Registers the input engine calls on @p app, for scripts: `rtype.input.addAction`, `rtype.input.bind`,
/// `rtype.input.bindAxis`, `rtype.input.bindVector` and `rtype.input.read`, each on the `rtype.Input` resource. They
/// throw an InputException naming an unknown action, control or action type. Called by the InputPlugin.
void addInputCalls(ecs::App& app);

}  // namespace rtype::plugins::input
