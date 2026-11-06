/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputForms
*/

#include "InputForms.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "FormTable.hpp"
#include "SetupCalls.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/luau/CallContext.hpp"
#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/Library.hpp"
#include "rtype/luau/LibraryBuilder.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"

namespace rtype::scripting::setup {
namespace {
// The calls of the input plugin, as it names them (scripting does not depend on the plugins).
constexpr std::string_view kAddActionCall = "rtype.input.addAction";
constexpr std::string_view kBindCall = "rtype.input.bind";
constexpr std::string_view kBindAxisCall = "rtype.input.bindAxis";
constexpr std::string_view kBindVectorCall = "rtype.input.bindVector";
// The call of the replication plugin that declares an action of the network InputLayout.
constexpr std::string_view kAddInputCall = "rtype.replication.addInput";

constexpr std::string_view kActionForm = "engine.input.action";

constexpr std::array<std::string_view, 1> kActionOptions{"bind"};
constexpr std::array<std::string_view, 10> kBindingKeys{
    "control", "scale", "negative", "positive", "up", "down", "left", "right", "x", "y",
};

constexpr std::array<std::string_view, 1> kControlKeys{"control"};
constexpr std::array<std::string_view, 1> kScaleKeys{"scale"};
constexpr std::array<std::string_view, 2> kAxisKeys{"negative", "positive"};
constexpr std::array<std::string_view, 4> kDirectionKeys{"up", "down", "left", "right"};
constexpr std::array<std::string_view, 2> kStickKeys{"x", "y"};

/// @brief A shape a binding table can have, and the call it makes.
struct BindingShape {
    std::string_view call;                       ///< The call that binds it.
    std::span<const std::string_view> controls;  ///< Its keys, every one a control name.
    std::span<const std::string_view> numbers;   ///< Its optional keys that are numbers.
};

constexpr std::array kBindingShapes{
    BindingShape{.call = kBindCall, .controls = kControlKeys, .numbers = kScaleKeys},
    BindingShape{.call = kBindAxisCall, .controls = kAxisKeys, .numbers = {}},
    BindingShape{.call = kBindVectorCall, .controls = kDirectionKeys, .numbers = {}},
    BindingShape{.call = kBindVectorCall, .controls = kStickKeys, .numbers = {}},
};

/// @brief A call the form makes.
struct PlannedCall {
    std::string_view call;  ///< The call.
    CallFields fields;      ///< Its fields.
};

/// @return Whether @p binding has every control key of @p shape and no key of another shape.
[[nodiscard]] bool hasShape(const FormTable& binding, const BindingShape& shape) {
    if (!std::ranges::all_of(shape.controls, [&binding](const std::string_view key) { return binding.has(key); })) {
        return false;
    }
    const auto numbers = std::ranges::count_if(shape.numbers, [&binding](const auto key) { return binding.has(key); });
    return binding.size() == shape.controls.size() + static_cast<std::size_t>(numbers);
}

/// @return The call that binds the @p number th binding of `bind`, @p value.
[[nodiscard]] PlannedCall readBinding(const std::string& action, const std::size_t number, const luau::Value& value) {
    if (const auto* control = value.tryAs<std::string>(); control != nullptr) {
        return PlannedCall{.call = kBindCall, .fields = {{"action", luau::Value{action}}, {"control", value}}};
    }
    const FormTable binding{std::format("{}: binding {}", kActionForm, number), value, kBindingKeys};
    const BindingShape* shape = nullptr;
    for (const BindingShape& candidate : kBindingShapes) {
        if (hasShape(binding, candidate)) {
            shape = &candidate;
            break;
        }
    }
    if (shape == nullptr) {
        binding.fail(
            "expected a control name, {control, scale}, {negative, positive}, {up, down, left, right} or {x, y}");
    }
    PlannedCall planned{.call = shape->call, .fields = {{"action", luau::Value{action}}}};
    for (const std::string_view key : shape->controls) {
        auto control = binding.string(key);
        if (!control) {
            binding.fail(std::format("'{}' must be a control name", key));
        }
        planned.fields.emplace_back(key, luau::Value{std::move(*control)});
    }
    for (const std::string_view key : shape->numbers) {
        if (const auto number = binding.number(key)) {
            planned.fields.emplace_back(key, luau::Value{*number});
        }
    }
    return planned;
}

luau::ValuePack action(const SetupCalls& calls, const luau::CallContext& context) {
    checkArgumentCount(kActionForm, context, 3);
    const luau::Value name = argumentAt(context, 0);
    const auto* text = name.tryAs<std::string>();
    if (text == nullptr || text->empty()) {
        throw luau::exceptions::RuntimeError{std::format("{}: expected the action's name first", kActionForm)};
    }
    const luau::Value type = argumentAt(context, 1);
    if (!type.is<luau::Value::Nil>() && !type.is<std::string>()) {
        throw luau::exceptions::RuntimeError{
            std::format(R"({}: the type must be "button", "axis" or "vector2")", kActionForm)};
    }
    const FormTable options =
        FormTable::optional(std::format("{}: options", kActionForm), argumentAt(context, 2), kActionOptions);

    std::vector<PlannedCall> planned;
    CallFields addAction{{"name", name}};
    if (type.is<std::string>()) {
        addAction.emplace_back("type", type);
    }
    planned.push_back(PlannedCall{.call = kAddActionCall, .fields = std::move(addAction)});
    const std::vector<luau::Value> bindings = options.list("bind").value_or(std::vector<luau::Value>{});
    for (std::size_t index = 0; index < bindings.size(); ++index) {
        planned.push_back(readBinding(*text, index + 1, bindings.at(index)));
    }

    if (calls.has(kAddActionCall)) {
        for (const PlannedCall& call : planned) {
            static_cast<void>(calls.invoke(kActionForm, call.call, call.fields));
        }
    }
    // On every side with the replication plugin, so the server and its clients agree on the PlayerInput layout.
    if (calls.has(kAddInputCall)) {
        CallFields addInput{{"name", name}};
        if (type.is<std::string>()) {
            addInput.emplace_back("kind", type);
        }
        static_cast<void>(calls.invoke(kActionForm, kAddInputCall, addInput));
    }
    return luau::ValuePack{};
}
}  // namespace

luau::Library buildInputForms(luau::Runtime& runtime, ecs::App& app) {
    const SetupCalls calls{runtime, app};
    luau::LibraryBuilder builder{runtime};
    builder.setName("input");
    builder.function("action").setHandler([calls](const luau::CallContext& context) { return action(calls, context); });
    return builder.build();
}
}  // namespace rtype::scripting::setup
