/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldState
*/

#include "WorldState.hpp"

#include <format>
#include <string_view>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/event/EventInfo.hpp"
#include "rtype/ecs/event/EventRegistry.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
#include "rtype/ecs/resource/ResourceRegistry.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"

namespace rtype::ecs::detail {
namespace {
/// @return The kind at the start of a message: "Component", "Resource", "Event".
[[nodiscard]] std::string_view toLabel(TypeKind kind) noexcept {
    switch (kind) {
        case TypeKind::kComponent:
            return KindTraits<ComponentInfo>::kLabel;
        case TypeKind::kResource:
            return KindTraits<ResourceInfo>::kLabel;
        case TypeKind::kEvent:
            return KindTraits<EventInfo>::kLabel;
    }
    return "Type";
}

/// @return The kind inside a message, with its article: "a component", "a resource", "an event".
[[nodiscard]] std::string_view toNoun(TypeKind kind) noexcept {
    switch (kind) {
        case TypeKind::kComponent:
            return "a component";
        case TypeKind::kResource:
            return "a resource";
        case TypeKind::kEvent:
            return "an event";
    }
    return "a type";
}
}  // namespace

Result<void> WorldState::checkNameFree(std::string_view name, TypeKind kind) const {
    TypeKind owner = kind;
    if (kind != TypeKind::kComponent && components.getRegistry().findByName(name) != nullptr) {
        owner = TypeKind::kComponent;
    } else if (kind != TypeKind::kResource && resources.getRegistry().findByName(name) != nullptr) {
        owner = TypeKind::kResource;
    } else if (kind != TypeKind::kEvent && events.getRegistry().findByName(name) != nullptr) {
        owner = TypeKind::kEvent;
    }
    if (owner != kind) {
        return fail(ErrorKind::kNameCollision,
                    std::format("{} '{}': the name is already used by {}", toLabel(kind), name, toNoun(owner)));
    }
    return {};
}
}  // namespace rtype::ecs::detail
