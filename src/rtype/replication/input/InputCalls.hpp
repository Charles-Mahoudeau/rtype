/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputCalls
*/

#pragma once

#include <string_view>
#include <type_traits>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/call/CallTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/FieldTypeOf.hpp"
#include "rtype/ecs/type/String.hpp"
#include "rtype/replication/Export.hpp"

namespace rtype::replication::input {
/// @brief Two floats, the value of a vector action: replication does not depend on glm, so the call results use this
/// instead of glm::vec2. Reflected as ecs::FieldType::kVec2, a script reads it as `{x, y}`.
struct InputVector {
    float x{0.0F};  ///< The horizontal value, in [-1, 1].
    float y{0.0F};  ///< The vertical value, in [-1, 1], up positive.
};
}  // namespace rtype::replication::input

/// @brief An InputVector field is reflected as kVec2.
template <>
struct rtype::ecs::FieldTypeOf<rtype::replication::input::InputVector>
    : std::integral_constant<rtype::ecs::FieldType, rtype::ecs::FieldType::kVec2> {};
static_assert(sizeof(rtype::replication::input::InputVector) == 8 &&
                  alignof(rtype::replication::input::InputVector) == 4,
              "InputVector must match FieldType::kVec2");

namespace rtype::replication::input {
inline constexpr std::string_view kAddInputCall = "rtype.replication.addInput";    ///< InputLayout::add().
inline constexpr std::string_view kReadInputCall = "rtype.replication.readInput";  ///< Reads a PlayerInput by name.

/// @brief The arguments of `rtype.replication.addInput`: declares an action of the network InputLayout.
struct AddInputArguments {
    ecs::String name;            ///< The action's name, 1 to 255 bytes, not declared yet.
    ecs::String kind{"button"};  ///< What it carries: "button", "axis" or "vector2".
};
RTYPE_ECS_CALL_DATA(AddInputArguments, "rtype.replication.AddInputArguments", name, kind);

/// @brief The arguments of `rtype.replication.readInput`: an action of an entity's PlayerInput.
struct ReadInputArguments {
    ecs::Entity entity;  ///< The entity, with a PlayerInput (the server's ships, filled from their client's inputs).
    ecs::String action;  ///< The action's name, as declared in the InputLayout.
};
RTYPE_ECS_CALL_DATA(ReadInputArguments, "rtype.replication.ReadInputArguments", entity, action);

/// @brief The result of `rtype.replication.readInput`: the fields of the action's kind are set, the others are zero.
struct InputState {
    bool held{false};      ///< A button: held now (InputLayout::isHeld()).
    bool pressed{false};   ///< A button: held now and not at the input before (InputLayout::isPressed()).
    bool released{false};  ///< A button: held at the input before and not now (InputLayout::isReleased()).
    float axis{0.0F};      ///< An axis: its value, in [-1, 1] (InputLayout::readAxis()).
    InputVector vector;    ///< A vector: its value (InputLayout::readVector()).
};
RTYPE_ECS_CALL_DATA(InputState, "rtype.replication.InputState", held, pressed, released, axis, vector);

/// @brief Registers the input engine calls on @p app, for scripts, on the `net.InputLayout` resource:
/// `rtype.replication.addInput`, which declares an action while the layout is not frozen (scripts declare them while
/// they load, through `engine.input.action`), and `rtype.replication.readInput`, which reads an action of an entity's
/// PlayerInput. Both throw an exceptions::InputLayoutException on an unknown action or kind, a frozen layout, or an
/// entity without a PlayerInput. Called by the ReplicationPlugin.
RTYPE_REPLICATION_API void addInputCalls(ecs::App& app);
}  // namespace rtype::replication::input
