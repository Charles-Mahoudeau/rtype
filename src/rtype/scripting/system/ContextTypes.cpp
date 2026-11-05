/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ContextTypes
*/

#include "ContextTypes.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>

#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/Result.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/Userdata.hpp"
#include "rtype/luau/Usertype.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/scripting/view/ComponentView.hpp"

namespace rtype::scripting::system {
namespace {
/// @return The view of the declared resource called `name`, or `nil` while it has no value.
/// @throws luau::exceptions::RuntimeError If the handle is stale or the system did not declare the resource.
[[nodiscard]] luau::Value viewOf(const luau::Runtime& runtime, const ResourcesHandle& handle,
                                 const std::string_view name) {
    const auto scope = handle.scope.lock();
    if (scope == nullptr || scope->generation != handle.generation) {
        throw luau::exceptions::RuntimeError{
            "ctx.resources is no longer valid: it only lives during the run that received it"};
    }
    const auto& terms = handle.system->resources;
    const auto found = std::ranges::find(terms, name, [](const ResourceTerm& term) { return term.type->name; });
    if (found == terms.end()) {
        throw luau::exceptions::RuntimeError{std::format(
            "resource '{}' is not declared by system '{}': list it under 'resources'", name, handle.system->name)};
    }
    const auto index = static_cast<std::size_t>(std::distance(terms.begin(), found));
    if (!handle.resources->has(index)) {
        return luau::Value{luau::Value::Nil{}};
    }
    std::span<std::byte> bytes;
    if (found->write) {
        bytes = handle.resources->getMut(index);
    } else {
        const std::span<const std::byte> read = handle.resources->get(index);
        // The view refuses writes, so the bytes are never written through it.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        bytes = {const_cast<std::byte*>(read.data()), read.size()};
    }
    auto view = runtime.usertype<view::ComponentView>().create(view::ComponentView{
        .type = found->type,
        .bytes = bytes,
        .scope = scope.get(),
        .generation = handle.generation,
        .readOnly = !found->write,
    });
    if (!view) {
        throw luau::exceptions::RuntimeError{std::string{view.error().message()}};
    }
    return luau::Value{std::move(*view)};
}
}  // namespace

luau::Result<> registerContextTypes(luau::Runtime& runtime) {
    auto resourcesType = runtime.usertype<ResourcesHandle>("Resources");
    if (!resourcesType) {
        return tl::unexpected{std::move(resourcesType.error())};
    }
    resourcesType
        ->index([&runtime](const ResourcesHandle& handle, const std::string_view name) {
            return viewOf(runtime, handle, name);
        })
        .tostring([](const ResourcesHandle& /*handle*/) { return std::string{"Resources"}; });
    return {};
}
}  // namespace rtype::scripting::system
