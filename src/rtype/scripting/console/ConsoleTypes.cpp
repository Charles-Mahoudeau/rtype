/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ConsoleTypes
*/

#include "ConsoleTypes.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Usertype.hpp"
#include "rtype/scripting/view/FieldAccess.hpp"

namespace rtype::scripting::console {

namespace {
constexpr std::string_view kKeys = "__keys";  ///< The pseudo-field listing the names a completion offers.

/// @brief A console line's handle on an entity.
struct ConsoleEntity {
    ConsoleAccess access;       ///< What the line may reach.
    std::uint64_t generation;   ///< The scope's generation when it was made.
    ecs::Entity entity;         ///< The entity.
};

/// @brief A console line's handle on one component of an entity.
struct ConsoleComponent {
    ConsoleAccess access;       ///< What the line may reach.
    std::uint64_t generation;   ///< The scope's generation when it was made.
    ecs::Entity entity;         ///< The entity.
    ecs::ComponentId id;        ///< The component.
};

/// @brief Raises a Luau error unless the line that made the handle is still running.
void checkAlive(const ConsoleAccess& access, const std::uint64_t generation) {
    if (access.scope->generation != generation) {
        throw luau::exceptions::RuntimeError{"this handle is no longer valid: it lives for the line that made it"};
    }
}

/// @return The last part of a component name (`Ship` for `game.Ship`).
std::string_view shortName(const std::string_view name) {
    const std::size_t dot = name.rfind('.');
    return dot == std::string_view::npos ? name : name.substr(dot + 1);
}

/// @return The ids of the components the entity has.
std::vector<ecs::ComponentId> componentsOf(const ConsoleAccess& access, const ecs::Entity entity) {
    std::vector<ecs::ComponentId> ids;
    const ecs::Components& components = access.world->getComponents();
    const std::size_t count = components.getRegistry().getCount();
    for (std::size_t index = 0; index < count; ++index) {
        const auto id = static_cast<ecs::ComponentId>(index);
        if (components.getRegistry().findById(id) != nullptr && components.has(entity, id)) {
            ids.push_back(id);
        }
    }
    return ids;
}

/// @brief The component a name stands for, or why none does.
struct Match {
    std::optional<ecs::ComponentId> id;  ///< The component; none when there is none or it is ambiguous.
    std::string problem;                 ///< Why there is none.
};

/// @return The component @p name stands for: its whole name, or its last part if only one component ends with it
/// (among the ones the entity has, if several do).
Match match(const ConsoleAccess& access, const ecs::Entity entity, const std::string_view name) {
    const ecs::Components& components = access.world->getComponents();
    if (const auto exact = components.find(name)) {
        return {.id = *exact, .problem = {}};
    }
    const std::size_t count = components.getRegistry().getCount();
    std::vector<ecs::ComponentId> known;
    std::vector<ecs::ComponentId> owned;
    for (std::size_t index = 0; index < count; ++index) {
        const auto id = static_cast<ecs::ComponentId>(index);
        const ecs::ComponentInfo* info = components.getRegistry().findById(id);
        if (info != nullptr && shortName(info->type.name) == name) {
            known.push_back(id);
            if (components.has(entity, id)) {
                owned.push_back(id);
            }
        }
    }
    if (known.empty()) {
        return {.id = std::nullopt, .problem = std::format("no component named '{}'", name)};
    }
    const std::vector<ecs::ComponentId>& candidates = known.size() == 1 ? known : owned;
    if (candidates.size() == 1) {
        return {.id = candidates.front(), .problem = {}};
    }
    std::string names;
    for (const ecs::ComponentId id : known) {
        names += std::format("{}'{}'", names.empty() ? "" : ", ", components.getRegistry().findById(id)->type.name);
    }
    return {.id = std::nullopt, .problem = std::format("'{}' is ambiguous, write the whole name: {}", name, names)};
}

/// @return A Luau list of @p names.
luau::Value listOf(const luau::Runtime& runtime, const std::vector<std::string>& names) {
    auto list = luau::Table::create(runtime);
    if (!list) {
        throw luau::exceptions::RuntimeError{"cannot make a list"};
    }
    std::size_t position = 1;
    for (const std::string& name : names) {
        static_cast<void>(list->setAt(position++, luau::Value{name}));
    }
    return luau::Value{*list};
}

/// @return The registered info of @p id.
const ecs::ComponentInfo& infoOf(const ConsoleAccess& access, const ecs::ComponentId id) {
    return *access.world->getComponents().getRegistry().findById(id);
}

/// @return The field of @p info named @p name; raises a Luau error when there is none.
const ecs::FieldInfo& findField(const ecs::ComponentInfo& info, const std::string_view name) {
    for (const ecs::FieldInfo& field : info.type.fields) {
        if (field.name == name) {
            return field;
        }
    }
    throw luau::exceptions::RuntimeError{std::format("component '{}' has no field '{}'", info.type.name, name)};
}

/// @brief Raises a Luau error unless the entity is alive and has the component.
void checkPresent(const ConsoleComponent& handle) {
    checkAlive(handle.access, handle.generation);
    if (!handle.access.world->isAlive(handle.entity) ||
        !handle.access.world->getComponents().has(handle.entity, handle.id)) {
        throw luau::exceptions::RuntimeError{
            std::format("the entity no longer has '{}'", infoOf(handle.access, handle.id).type.name)};
    }
}

luau::Result<luau::Usertype<ConsoleComponent>> registerComponent(luau::Runtime& runtime) {
    auto type = runtime.usertype<ConsoleComponent>("ConsoleComponent");
    if (!type) {
        return tl::unexpected{std::move(type.error())};
    }
    const view::FieldReadContext context{.runtime = &runtime,
                                         .entities = runtime.usertype<ecs::Entity>(),
                                         .assetIds = runtime.usertype<core::asset::AssetId>()};
    type->index([&runtime, context](const ConsoleComponent& handle, const std::string_view name) {
            checkPresent(handle);
            const ecs::ComponentInfo& info = infoOf(handle.access, handle.id);
            if (name == kKeys) {
                std::vector<std::string> names;
                for (const ecs::FieldInfo& field : info.type.fields) {
                    names.push_back(field.name);
                }
                return listOf(runtime, names);
            }
            const ecs::FieldInfo& field = findField(info, name);
            auto value = view::readField(info.type.name, field,
                                         handle.access.world->getComponents().get(handle.entity, handle.id), context);
            if (!value) {
                throw luau::exceptions::RuntimeError{std::string{value.error().message()}};
            }
            return std::move(*value);
        })
        .newIndex([](const ConsoleComponent& handle, const std::string_view name, const luau::Value& value) {
            checkPresent(handle);
            const ecs::ComponentInfo& info = infoOf(handle.access, handle.id);
            if (!handle.access.writable) {
                throw luau::exceptions::RuntimeError{std::format(
                    "cannot write '{}': the console only reads here (the server owns the data)", name)};
            }
            const ecs::FieldInfo& field = findField(info, name);
            // Through the access that marks the value changed, so the systems that watch it see it.
            auto written = view::writeField(info.type.name, field,
                                            handle.access.world->getComponents().getMut(handle.entity, handle.id), value);
            if (!written) {
                throw luau::exceptions::RuntimeError{std::string{written.error().message()}};
            }
        })
        .tostring([](const ConsoleComponent& handle) {
            return std::format("ConsoleComponent({})", infoOf(handle.access, handle.id).type.name);
        });
    return *type;
}

luau::Result<> registerEntity(luau::Runtime& runtime, const luau::Usertype<ConsoleComponent>& components) {
    auto type = runtime.usertype<ConsoleEntity>("ConsoleEntity");
    if (!type) {
        return tl::unexpected{std::move(type.error())};
    }
    type->index([&runtime, components](const ConsoleEntity& handle, const std::string_view name) {
            checkAlive(handle.access, handle.generation);
            if (!handle.access.world->isAlive(handle.entity)) {
                throw luau::exceptions::RuntimeError{"the entity is no longer alive"};
            }
            if (name == kKeys) {
                std::vector<std::string> names;
                for (const ecs::ComponentId id : componentsOf(handle.access, handle.entity)) {
                    const std::string_view brief = shortName(infoOf(handle.access, id).type.name);
                    // Only the names a dot reaches: the ones that stand for this component alone.
                    if (match(handle.access, handle.entity, brief).id == id) {
                        names.emplace_back(brief);
                    }
                }
                return listOf(runtime, names);
            }
            const Match found = match(handle.access, handle.entity, name);
            if (!found.id) {
                throw luau::exceptions::RuntimeError{found.problem};
            }
            if (!handle.access.world->getComponents().has(handle.entity, *found.id)) {
                return luau::Value{luau::Value::Nil{}};
            }
            auto made = components.create(ConsoleComponent{handle.access, handle.generation, handle.entity, *found.id});
            if (!made) {
                throw luau::exceptions::RuntimeError{std::string{made.error().message()}};
            }
            return luau::Value{std::move(*made)};
        })
        .tostring([](const ConsoleEntity& handle) {
            return std::format("Entity({}:{})", handle.entity.getIndex(), handle.entity.getGeneration());
        });
    return {};
}
}  // namespace

luau::Result<> registerConsoleTypes(luau::Runtime& runtime) {
    auto components = registerComponent(runtime);
    if (!components) {
        return tl::unexpected{std::move(components.error())};
    }
    return registerEntity(runtime, *components);
}

luau::Result<luau::Value> makeConsoleEntity(const luau::Runtime& runtime, const ConsoleAccess& access,
                                            const ecs::Entity entity) {
    auto made = runtime.usertype<ConsoleEntity>().create(ConsoleEntity{access, access.scope->generation, entity});
    if (!made) {
        return tl::unexpected{std::move(made.error())};
    }
    return luau::Value{std::move(*made)};
}
}  // namespace rtype::scripting::console
