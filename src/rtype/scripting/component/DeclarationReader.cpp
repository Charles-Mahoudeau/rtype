/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DeclarationReader
*/

#include "DeclarationReader.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>
#include <vector>

#include "ComponentDeclaration.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/scripting/Error.hpp"
#include "rtype/scripting/ErrorKind.hpp"
#include "rtype/scripting/Result.hpp"
#include "rtype/scripting/view/FieldAccess.hpp"

namespace rtype::scripting::component {
namespace {
struct FieldTypeName {
    std::string_view name;
    ecs::FieldType type;
};

constexpr std::array kFieldTypes{
    FieldTypeName{.name = "bool", .type = ecs::FieldType::kBool},
    FieldTypeName{.name = "i8", .type = ecs::FieldType::kI8},
    FieldTypeName{.name = "i16", .type = ecs::FieldType::kI16},
    FieldTypeName{.name = "i32", .type = ecs::FieldType::kI32},
    FieldTypeName{.name = "i64", .type = ecs::FieldType::kI64},
    FieldTypeName{.name = "u8", .type = ecs::FieldType::kU8},
    FieldTypeName{.name = "u16", .type = ecs::FieldType::kU16},
    FieldTypeName{.name = "u32", .type = ecs::FieldType::kU32},
    FieldTypeName{.name = "u64", .type = ecs::FieldType::kU64},
    FieldTypeName{.name = "f32", .type = ecs::FieldType::kF32},
    FieldTypeName{.name = "f64", .type = ecs::FieldType::kF64},
    FieldTypeName{.name = "vec2", .type = ecs::FieldType::kVec2},
    FieldTypeName{.name = "vec3", .type = ecs::FieldType::kVec3},
    FieldTypeName{.name = "vec4", .type = ecs::FieldType::kVec4},
    FieldTypeName{.name = "entity", .type = ecs::FieldType::kEntity},
    FieldTypeName{.name = "assetId", .type = ecs::FieldType::kAssetId},
    FieldTypeName{.name = "string", .type = ecs::FieldType::kString},
};

/// @brief What the reader works on: the component's name, for the messages, and its table.
struct Source {
    std::string_view component;
    luau::Table table;  ///< A handle on the table, cheap to copy.
};

[[nodiscard]] tl::unexpected<Error> invalid(const std::string_view component, const std::string& what) {
    return fail(ErrorKind::kInvalidDeclaration, std::format("Component '{}': {}", component, what));
}

/// @brief The entries of a table whose keys are all strings, sorted by key.
[[nodiscard]] Result<std::vector<luau::Table::Entry>> namedEntries(const std::string_view component,
                                                                   const luau::Table& table,
                                                                   const std::string_view what) {
    auto entries = table.entries();
    if (!entries) {
        return invalid(component, std::format("cannot read {}: {}", what, entries.error().message()));
    }
    for (const auto& [key, value] : *entries) {
        if (!key.is<std::string>()) {
            return invalid(component, std::format("the keys of {} must be strings", what));
        }
    }
    std::ranges::sort(*entries, [](const luau::Table::Entry& left, const luau::Table::Entry& right) {
        return left.first.as<std::string>() < right.first.as<std::string>();
    });
    return std::move(*entries);
}

[[nodiscard]] Result<ecs::FieldType> readFieldType(const std::string_view component, const std::string_view field,
                                                   const luau::Value& value) {
    const auto* name = value.tryAs<std::string>();
    if (name == nullptr) {
        return invalid(component, std::format("the type of field '{}' must be a string", field));
    }
    // An array iterator is a pointer on libstdc++ and libc++ but a class on MSVC, so it cannot be spelled `auto*`.
    // NOLINTNEXTLINE(readability-qualified-auto)
    if (const auto found = std::ranges::find(kFieldTypes, *name, &FieldTypeName::name); found != kFieldTypes.end()) {
        return found->type;
    }
    return invalid(component, std::format("field '{}' has an unknown type '{}'", field, *name));
}

[[nodiscard]] Result<std::vector<FieldDeclaration>> readFields(const Source& source) {
    auto entries = namedEntries(source.component, source.table, "'fields'");
    if (!entries) {
        return tl::unexpected{std::move(entries.error())};
    }
    std::vector<FieldDeclaration> fields;
    for (const auto& [key, value] : *entries) {
        const auto& name = key.as<std::string>();
        const auto type = readFieldType(source.component, name, value);
        if (!type) {
            return tl::unexpected{type.error()};
        }
        fields.push_back(FieldDeclaration{.name = name, .type = *type, .defaultValue = {}});
    }
    return fields;
}

/// @brief Sets the default of every field the `defaults` table names, as the bytes of the field's type.
[[nodiscard]] Result<void> readDefaults(const Source& source, std::vector<FieldDeclaration>& fields) {
    const auto entries = namedEntries(source.component, source.table, "'defaults'");
    if (!entries) {
        return tl::unexpected{entries.error()};
    }
    for (const auto& [key, value] : *entries) {
        const auto& name = key.as<std::string>();
        const auto field = std::ranges::find(fields, name, &FieldDeclaration::name);
        if (field == fields.end()) {
            return invalid(source.component, std::format("'defaults' names '{}', which is not a field", name));
        }
        const ecs::FieldInfo info{.name = name, .type = field->type, .offset = 0, .count = 1};
        field->defaultValue.assign(ecs::getFieldLayout(field->type).size, std::byte{0});
        if (auto written = view::writeField(source.component, info, field->defaultValue, value); !written) {
            return written;
        }
    }
    return {};
}

[[nodiscard]] Result<ecs::ReplicationMode> readReplication(const std::string_view component, const luau::Value& value) {
    const auto* mode = value.tryAs<std::string>();
    if (mode != nullptr && *mode == "none") {
        return ecs::ReplicationMode::kNone;
    }
    if (mode != nullptr && *mode == "everyChange") {
        return ecs::ReplicationMode::kEveryChange;
    }
    if (mode != nullptr && *mode == "spawnOnly") {
        return ecs::ReplicationMode::kSpawnOnly;
    }
    return invalid(component, R"('replication' must be "none", "everyChange" or "spawnOnly")");
}

[[nodiscard]] Result<ecs::Presence> readPresence(const std::string_view component, const luau::Value& value) {
    const auto* mode = value.tryAs<std::string>();
    if (mode != nullptr && *mode == "both") {
        return ecs::Presence::kBoth;
    }
    if (mode != nullptr && *mode == "serverOnly") {
        return ecs::Presence::kServerOnly;
    }
    if (mode != nullptr && *mode == "clientOnly") {
        return ecs::Presence::kClientOnly;
    }
    return invalid(component, R"('presence' must be "both", "serverOnly" or "clientOnly")");
}

/// @brief Reads `replication` or `presence` into the declaration; any other option is an error.
[[nodiscard]] Result<void> readPolicy(ComponentDeclaration& declaration, const std::string& option,
                                      const luau::Value& value) {
    if (option == "replication") {
        const auto replication = readReplication(declaration.name, value);
        if (!replication) {
            return tl::unexpected{replication.error()};
        }
        declaration.replication = *replication;
    } else if (option == "presence") {
        const auto presence = readPresence(declaration.name, value);
        if (!presence) {
            return tl::unexpected{presence.error()};
        }
        declaration.presence = *presence;
    } else {
        return invalid(
            declaration.name,
            std::format("unknown option '{}' (expected 'fields', 'defaults', 'replication' or 'presence')", option));
    }
    return {};
}
}  // namespace

Result<ComponentDeclaration> readComponentDeclaration(std::string name, const luau::Table& spec) {
    const auto entries = namedEntries(name, spec, "the declaration");
    if (!entries) {
        return tl::unexpected{entries.error()};
    }

    ComponentDeclaration declaration{
        .name = std::move(name),
        .fields = {},
        .replication = ecs::ReplicationMode::kNone,
        .presence = ecs::Presence::kBoth,
    };
    std::optional<luau::Table> defaults;
    bool hasFields = false;
    for (const auto& [key, value] : *entries) {
        const auto& option = key.as<std::string>();
        if (option == "fields" || option == "defaults") {
            const auto* table = value.tryAs<luau::Table>();
            if (table == nullptr) {
                return invalid(declaration.name, std::format("'{}' must be a table", option));
            }
            if (option == "defaults") {
                defaults = *table;
                continue;
            }
            auto fields = readFields({.component = declaration.name, .table = *table});
            if (!fields) {
                return tl::unexpected{std::move(fields.error())};
            }
            declaration.fields = std::move(*fields);
            hasFields = true;
        } else if (auto applied = readPolicy(declaration, option, value); !applied) {
            return tl::unexpected{std::move(applied.error())};
        }
    }
    if (!hasFields) {
        return invalid(declaration.name, "needs a 'fields' table");
    }
    if (defaults) {
        if (auto read = readDefaults({.component = declaration.name, .table = *defaults}, declaration.fields); !read) {
            return tl::unexpected{std::move(read.error())};
        }
    }
    return declaration;
}
}  // namespace rtype::scripting::component
