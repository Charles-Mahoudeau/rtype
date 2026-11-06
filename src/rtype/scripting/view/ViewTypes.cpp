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
#include "core/asset/AssetId.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/Result.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/Usertype.hpp"
#include "rtype/luau/Value.hpp"

namespace rtype::scripting::view {
namespace {
/// @brief Raises a Luau error unless the view can still be used.
void checkAlive(const ComponentView& view) {
    if (view.type == nullptr || !view.scope) {
        throw luau::exceptions::RuntimeError{"the view is not bound to a component"};
    }
    if (view.scope->generation != view.generation) {
        throw luau::exceptions::RuntimeError{std::format(
            "the view of '{}' is no longer valid: a view only lives during the run that received it", view.type->name)};
    }
}

/// @brief Finds a field of the view's component, raising a Luau error when there is none.
const ecs::FieldInfo& findField(const ComponentView& view, const std::string_view name) {
    const auto& fields = view.type->fields;
    const auto field = std::ranges::find(fields, name, &ecs::FieldInfo::name);
    if (field == fields.end()) {
        throw luau::exceptions::RuntimeError{std::format("component '{}' has no field '{}'", view.type->name, name)};
    }
    return *field;
}
/// @brief Registers `Entity`: an opaque handle scripts compare, print and use as a table key through `bits`.
/// @return The usertype, or the error of the registration.
luau::Result<luau::Usertype<ecs::Entity>> registerEntityType(luau::Runtime& runtime) {
    auto entityType = runtime.usertype<ecs::Entity>("Entity");
    if (!entityType) {
        return tl::unexpected{std::move(entityType.error())};
    }
    entityType->eq([](const ecs::Entity& lhs, const ecs::Entity& rhs) { return lhs == rhs; })
        .tostring([](const ecs::Entity& entity) {
            return std::format("Entity({}:{})", entity.getIndex(), entity.getGeneration());
        })
        .getter("bits", [](const ecs::Entity& entity) { return static_cast<double>(entity.getBits()); });
    return *entityType;
}

/// @brief Registers `AssetId`: the hash of an asset's path, compared and printed in hex.
/// @return The usertype, or the error of the registration.
luau::Result<luau::Usertype<core::asset::AssetId>> registerAssetIdType(luau::Runtime& runtime) {
    auto assetIdType = runtime.usertype<core::asset::AssetId>("AssetId");
    if (!assetIdType) {
        return tl::unexpected{std::move(assetIdType.error())};
    }
    assetIdType->eq([](const core::asset::AssetId& lhs, const core::asset::AssetId& rhs) { return lhs == rhs; })
        .tostring([](const core::asset::AssetId& id) { return std::format("AssetId({:016x})", id.value); });
    return *assetIdType;
}

/// @brief Registers `ComponentView`: reads and writes the fields of a component in place.
/// @param context What reading a field needs to build its value.
/// @return Nothing, or the error of the registration.
luau::Result<> registerComponentViewType(luau::Runtime& runtime, const FieldReadContext& context) {
    auto viewType = runtime.usertype<ComponentView>("ComponentView");
    if (!viewType) {
        return tl::unexpected{std::move(viewType.error())};
    }
    viewType
        ->index([context](const ComponentView& view, const std::string_view name) {
            checkAlive(view);
            const auto& field = findField(view, name);
            auto value = readField(view.type->name, field, view.bytes, context);
            if (!value) {
                throw luau::exceptions::RuntimeError{std::string{value.error().message()}};
            }
            return std::move(*value);
        })
        .newIndex([](const ComponentView& view, const std::string_view name, const luau::Value& value) {
            checkAlive(view);
            if (view.readOnly) {
                throw luau::exceptions::RuntimeError{
                    std::format("cannot write '{}': the view of '{}' is read-only", name, view.type->name)};
            }
            const auto& field = findField(view, name);
            if (auto written = writeField(view.type->name, field, view.bytes, value); !written) {
                throw luau::exceptions::RuntimeError{std::string{written.error().message()}};
            }
        })
        .tostring([](const ComponentView& view) {
            checkAlive(view);
            return std::format("View({})", view.type->name);
        });
    return {};
}
}  // namespace

luau::Result<> registerViewTypes(luau::Runtime& runtime) {
    auto entities = registerEntityType(runtime);
    if (!entities) {
        return tl::unexpected{std::move(entities.error())};
    }
    auto assetIds = registerAssetIdType(runtime);
    if (!assetIds) {
        return tl::unexpected{std::move(assetIds.error())};
    }
    return registerComponentViewType(runtime, {.runtime = &runtime, .entities = *entities, .assetIds = *assetIds});
}
}  // namespace rtype::scripting::view
