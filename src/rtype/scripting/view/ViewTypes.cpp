/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ViewTypes
*/

#include "ViewTypes.hpp"

#include <algorithm>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>

#include "ComponentView.hpp"
#include "FieldAccess.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/Value.hpp"

namespace rtype::scripting::view {
namespace {
/// @brief Raises a Luau error unless the view can still be used.
void checkAlive(const ComponentView& view) {
    if (view.scope == nullptr || view.scope->generation != view.generation) {
        throw luau::exceptions::RuntimeError{
            std::format("the view of '{}' is no longer valid: a view only lives during the run that received it",
                        view.info->type.name)};
    }
}

/// @brief Finds a field of the view's component, raising a Luau error when there is none.
const ecs::FieldInfo& findField(const ComponentView& view, const std::string_view name) {
    const auto& fields = view.info->type.fields;
    const auto field = std::ranges::find(fields, name, &ecs::FieldInfo::name);
    if (field == fields.end()) {
        throw luau::exceptions::RuntimeError{
            std::format("component '{}' has no field '{}'", view.info->type.name, name)};
    }
    return *field;
}
}  // namespace

luau::Result<> registerViewTypes(luau::Runtime& runtime) {
    auto entityType = runtime.usertype<ecs::Entity>("Entity");
    if (!entityType) {
        return tl::unexpected{std::move(entityType.error())};
    }
    const auto entities =
        entityType->eq([](const ecs::Entity& lhs, const ecs::Entity& rhs) { return lhs == rhs; })
            .tostring([](const ecs::Entity& entity) {
                return std::format("Entity({}:{})", entity.getIndex(), entity.getGeneration());
            })
            .getter("bits", [](const ecs::Entity& entity) { return static_cast<double>(entity.getBits()); });

    auto viewType = runtime.usertype<ComponentView>("ComponentView");
    if (!viewType) {
        return tl::unexpected{std::move(viewType.error())};
    }
    viewType
        ->index([entities](const ComponentView& view, const std::string_view name) {
            checkAlive(view);
            const auto& field = findField(view, name);
            auto value = readField(view.info->type.name, field, view.bytes, entities);
            if (!value) {
                throw luau::exceptions::RuntimeError{std::string{value.error().message()}};
            }
            return std::move(*value);
        })
        .newIndex([](const ComponentView& view, const std::string_view name, const luau::Value& value) {
            checkAlive(view);
            if (view.readOnly) {
                throw luau::exceptions::RuntimeError{
                    std::format("cannot write '{}': the view of '{}' is read-only", name, view.info->type.name)};
            }
            const auto& field = findField(view, name);
            if (auto written = writeField(view.info->type.name, field, view.bytes, value); !written) {
                throw luau::exceptions::RuntimeError{std::string{written.error().message()}};
            }
        })
        .tostring([](const ComponentView& view) { return std::format("View({})", view.info->type.name); });
    return {};
}
}  // namespace rtype::scripting::view
