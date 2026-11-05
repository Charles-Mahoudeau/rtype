/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentRegistration
*/

#include "ComponentRegistration.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <tl/expected.hpp>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ComponentDeclaration.hpp"
#include "ComponentLayout.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/scripting/ErrorKind.hpp"
#include "rtype/scripting/Result.hpp"

namespace rtype::scripting::component {
namespace {
[[nodiscard]] std::size_t alignUp(const std::size_t value, const std::size_t alignment) noexcept {
    return (value + alignment - 1) / alignment * alignment;
}

[[nodiscard]] Result<void> validateDeclaration(const ComponentDeclaration& declaration) {
    if (declaration.name.empty()) {
        return fail(ErrorKind::kInvalidDeclaration, "A component needs a name");
    }
    std::unordered_set<std::string> seen;
    for (const auto& [name, type, defaultValue] : declaration.fields) {
        if (name.empty()) {
            return fail(ErrorKind::kInvalidDeclaration,
                        std::format("Component '{}': a field needs a name", declaration.name));
        }
        if (!seen.insert(name).second) {
            return fail(ErrorKind::kInvalidDeclaration,
                        std::format("Component '{}': duplicate field '{}'", declaration.name, name));
        }
        if (const std::size_t size = ecs::getFieldLayout(type).size;
            !defaultValue.empty() && defaultValue.size() != size) {
            return fail(ErrorKind::kInvalidDeclaration,
                        std::format("Component '{}': the default of field '{}' is {} bytes, expected {}",
                                    declaration.name, name, defaultValue.size(), size));
        }
    }
    return {};
}
}  // namespace

Result<ComponentLayout> registerComponent(ecs::Components& components, const ComponentDeclaration& declaration) {
    if (Result<void> valid = validateDeclaration(declaration); !valid) {
        return tl::unexpected{std::move(valid.error())};
    }

    std::vector<const FieldDeclaration*> sorted;
    sorted.reserve(declaration.fields.size());
    for (const FieldDeclaration& field : declaration.fields) {
        sorted.push_back(&field);
    }
    std::ranges::sort(sorted, [](const FieldDeclaration* left, const FieldDeclaration* right) {
        const std::uint32_t leftAlignment = ecs::getFieldLayout(left->type).alignment;
        const std::uint32_t rightAlignment = ecs::getFieldLayout(right->type).alignment;
        return leftAlignment != rightAlignment ? leftAlignment > rightAlignment : left->name < right->name;
    });

    ComponentLayout layout;
    std::size_t offset = 0;
    for (const FieldDeclaration* field : sorted) {
        const auto [size, alignment] = ecs::getFieldLayout(field->type);
        offset = alignUp(offset, alignment);
        layout.fields.push_back(ecs::FieldInfo{
            .name = field->name, .type = field->type, .offset = static_cast<std::uint32_t>(offset), .count = 1});
        layout.alignment = std::max<std::size_t>(layout.alignment, alignment);
        offset += size;
    }
    layout.size = alignUp(offset, layout.alignment);

    layout.defaults.assign(layout.size, std::byte{0});
    for (std::size_t i = 0; i < sorted.size(); ++i) {
        std::ranges::copy(sorted.at(i)->defaultValue, layout.defaults.begin() + layout.fields.at(i).offset);
    }

    ecs::ComponentInfo info;
    info.type.name = declaration.name;
    info.type.size = layout.size;
    info.type.alignment = layout.alignment;
    info.type.replication = declaration.replication;
    info.type.presence = declaration.presence;
    info.type.fields = layout.fields;

    const ecs::Result<ecs::ComponentId> registered = components.registerType(std::move(info));
    if (!registered) {
        const ErrorKind kind = registered.error().getKind() == ecs::ErrorKind::kLayoutMismatch
                                   ? ErrorKind::kLayoutMismatch
                                   : ErrorKind::kRegistration;
        return fail(kind, std::string{registered.error().getMessage()});
    }
    layout.id = *registered;
    return layout;
}
}  // namespace rtype::scripting::component
