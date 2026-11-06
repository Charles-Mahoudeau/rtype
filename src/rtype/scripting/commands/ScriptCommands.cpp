/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptCommands
*/

#include "ScriptCommands.hpp"

#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <tl/expected.hpp>
#include <utility>
#include <vector>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/TypeHooks.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/Result.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Userdata.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/scripting/view/FieldAccess.hpp"

namespace rtype::scripting::commands {
namespace {
/// @brief A component value ready to be recorded.
struct Built {
    ecs::ComponentId id{};         ///< The component.
    std::vector<std::byte> bytes;  ///< Its defaults with the fields of the script written over them.
};

/// @brief Raises a Luau error unless the handle can still be used.
void checkAlive(const ScriptCommands& handle) {
    const auto scope = handle.scope.lock();
    if (scope == nullptr || scope->generation != handle.generation) {
        throw luau::exceptions::RuntimeError{
            "the commands are no longer valid: they only live during the run that received them"};
    }
}

/// @return A plain-data C++ component of the world.
/// @throws luau::exceptions::RuntimeError If the world has no such component, it is not plain data, or the system runs
/// on clients and the component is replicated or server-only.
[[nodiscard]] const ecs::ComponentInfo& requireEngineComponent(const ScriptCommands& handle, const std::string& name) {
    const ecs::ComponentInfo* info = handle.registry != nullptr ? handle.registry->findByName(name) : nullptr;
    if (info == nullptr) {
        throw luau::exceptions::RuntimeError{
            std::format("unknown component '{}': no script declared it and the world has none", name)};
    }
    const ecs::TypeHooks& hooks = info->type.hooks;
    if (hooks.destruct != nullptr || hooks.move != nullptr || hooks.copy != nullptr) {
        throw luau::exceptions::RuntimeError{
            std::format("cannot add '{}': it is not plain data (it owns memory of its own)", name)};
    }
    if (handle.clientSide &&
        (info->type.replication != ecs::ReplicationMode::kNone || info->type.presence == ecs::Presence::kServerOnly)) {
        throw luau::exceptions::RuntimeError{
            std::format("a client system cannot add '{}': it is replicated or server-only", name)};
    }
    return *info;
}

/// @return The bytes of a default-constructed value of a C++ type.
[[nodiscard]] std::vector<std::byte> defaultsOf(const ecs::TypeInfo& type) {
    std::vector<std::byte> bytes(type.size);
    if (type.hooks.construct != nullptr && !bytes.empty()) {
        type.hooks.construct(bytes.data());
    }
    return bytes;
}

/// @brief Builds the value of a component from its defaults and the fields of a table.
/// @throws luau::exceptions::RuntimeError If the component is unknown or refused, or a field is refused.
[[nodiscard]] Built build(const ScriptCommands& handle, const std::string& name, const luau::Table* fields) {
    const auto layout = handle.layouts->find(name);
    Built built;
    std::span<const ecs::FieldInfo> described;
    if (layout != handle.layouts->end()) {
        if (handle.clientSide && !layout->second.clientWritable) {
            throw luau::exceptions::RuntimeError{
                std::format("a client system cannot add '{}': it is replicated or server-only", name)};
        }
        built = Built{.id = layout->second.id, .bytes = layout->second.defaults};
        described = layout->second.fields;
    } else {
        const ecs::ComponentInfo& info = requireEngineComponent(handle, name);
        built = Built{.id = info.type.id, .bytes = defaultsOf(info.type)};
        described = info.type.fields;
    }
    if (fields != nullptr) {
        ecs::TypeInfo description;
        description.name = name;
        description.fields.assign(described.begin(), described.end());
        if (auto filled = view::fillFromTable(description, *fields, built.bytes); !filled) {
            throw luau::exceptions::RuntimeError{std::string{filled.error().message()}};
        }
    }
    return built;
}

/// @return The entity a script handed over, or a Luau error if the value is not an `Entity`.
[[nodiscard]] ecs::Entity toEntity(const luau::Userdata& userdata) {
    const auto* entity = userdata.tryAs<ecs::Entity>();
    if (entity == nullptr) {
        throw luau::exceptions::RuntimeError{"expected an Entity"};
    }
    return *entity;
}
}  // namespace

luau::Result<> registerCommandsType(luau::Runtime& runtime) {
    const auto entities = runtime.usertype<ecs::Entity>();
    auto commandsType = runtime.usertype<ScriptCommands>("Commands");
    if (!commandsType) {
        return tl::unexpected{std::move(commandsType.error())};
    }
    commandsType
        ->method("spawn",
                 [entities](const ScriptCommands& handle, const std::optional<luau::Table>& components) {
                     checkAlive(handle);
                     std::vector<Built> values;
                     if (components) {
                         auto entries = components->entries();
                         if (!entries) {
                             throw luau::exceptions::RuntimeError{std::string{entries.error().message()}};
                         }
                         for (const auto& [key, value] : *entries) {
                             const auto* name = key.tryAs<std::string>();
                             const auto* fields = value.tryAs<luau::Table>();
                             if (name == nullptr || fields == nullptr) {
                                 throw luau::exceptions::RuntimeError{
                                     "spawn expects a table of component names and tables of fields"};
                             }
                             values.push_back(build(handle, *name, fields));
                         }
                     }
                     // Everything is checked: nothing is recorded, nor an entity reserved, by a call that fails.
                     const auto commands = handle.commands->spawn();
                     for (const auto& [id, bytes] : values) {
                         handle.commands->add(commands.getEntity(), id, bytes);
                     }
                     auto created = entities.create(commands.getEntity());
                     if (!created) {
                         handle.commands->despawn(commands.getEntity());  // do not leave the reserved entity behind
                         throw luau::exceptions::RuntimeError{std::string{created.error().message()}};
                     }
                     return luau::Value{std::move(*created)};
                 })
        .method("add",
                [](const ScriptCommands& handle, const luau::Userdata& entity, const std::string& name,
                   const std::optional<luau::Table>& fields) {
                    checkAlive(handle);
                    const ecs::Entity target = toEntity(entity);
                    const auto [id, bytes] = build(handle, name, fields ? &*fields : nullptr);
                    handle.commands->add(target, id, bytes);
                })
        .method("setParent",
                [](const ScriptCommands& handle, const luau::Userdata& child, const luau::Userdata& parent) {
                    checkAlive(handle);
                    handle.commands->add(toEntity(child), ecs::Parent{.entity = toEntity(parent)});
                })
        .method("removeParent",
                [](const ScriptCommands& handle, const luau::Userdata& child) {
                    checkAlive(handle);
                    handle.commands->remove<ecs::Parent>(toEntity(child));
                })
        .method("despawn",
                [](const ScriptCommands& handle, const luau::Userdata& entity) {
                    checkAlive(handle);
                    handle.commands->despawn(toEntity(entity));
                })
        .tostring([](const ScriptCommands& /*handle*/) { return std::string{"Commands"}; });
    return {};
}
}  // namespace rtype::scripting::commands
