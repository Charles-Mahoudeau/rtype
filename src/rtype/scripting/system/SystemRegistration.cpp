/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SystemRegistration
*/

#include "SystemRegistration.hpp"

#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <tl/expected.hpp>
#include <utility>
#include <vector>

#include "RuntimeHandle.hpp"
#include "SystemDeclaration.hpp"
#include "SystemRun.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/event/EventInfo.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/exceptions/ScheduleException.hpp"
#include "rtype/ecs/query/QueryBuilder.hpp"
#include "rtype/ecs/query/QueryTerm.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/system/DynamicEvents.hpp"
#include "rtype/ecs/system/DynamicQueries.hpp"
#include "rtype/ecs/system/DynamicResources.hpp"
#include "rtype/ecs/system/QueryList.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/ScriptCallable.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/time/Time.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/scripting/ErrorKind.hpp"
#include "rtype/scripting/Result.hpp"
#include "rtype/scripting/view/ViewScope.hpp"

namespace rtype::scripting::system {
namespace {
/// @brief Adds a term to the builder.
void addTerm(ecs::QueryBuilder& builder, const ecs::TermKind kind, const ecs::ComponentId id) {
    switch (kind) {
        case ecs::TermKind::kRead:
            builder.read(id);
            break;
        case ecs::TermKind::kWrite:
            builder.write(id);
            break;
        case ecs::TermKind::kOptionalRead:
            builder.optionalRead(id);
            break;
        case ecs::TermKind::kOptionalWrite:
            builder.optionalWrite(id);
            break;
        case ecs::TermKind::kWith:
            builder.with(id);
            break;
        case ecs::TermKind::kWithout:
            builder.without(id);
            break;
        case ecs::TermKind::kAdded:
            builder.added(id);
            break;
        case ecs::TermKind::kChanged:
            builder.changed(id);
            break;
    }
}

/// @return The builder of the system's query, checked against the world's components, or the reason it is refused. A
/// system without terms (one that only uses resources and events) has no query: the list is empty.
[[nodiscard]] Result<std::vector<ecs::QueryBuilder>> makeQuery(ecs::Components& components,
                                                               const SystemDeclaration& declaration) {
    if (declaration.terms.empty()) {
        return std::vector<ecs::QueryBuilder>{};
    }
    ecs::QueryBuilder builder;
    for (const auto& [kind, component] : declaration.terms) {
        const auto id = components.find(component);
        if (!id) {
            return fail(ErrorKind::kInvalidDeclaration,
                        std::format("System '{}': unknown component '{}'", declaration.name, component));
        }
        addTerm(builder, kind, *id);
    }
    if (const auto built = builder.build(components); !built) {
        return fail(ErrorKind::kInvalidDeclaration,
                    std::format("System '{}': invalid query: {}", declaration.name, built.error().getMessage()));
    }
    std::vector<ecs::QueryBuilder> builders;
    builders.push_back(std::move(builder));
    return builders;
}

/// @return The data terms of the system's query, in the order the query yields them (the order of the declaration).
[[nodiscard]] std::vector<DataTerm> makeDataTerms(const ecs::Components& components,
                                                  const SystemDeclaration& declaration) {
    std::vector<DataTerm> terms;
    for (const auto& [kind, component] : declaration.terms) {
        if (ecs::isDataTerm(kind)) {
            // makeQuery() found every component of the declaration.
            terms.push_back(
                {.info = components.getRegistry().findByName(component), .readOnly = !ecs::isWriteTerm(kind)});
        }
    }
    return terms;
}

/// @return Why a type cannot be given to scripts as bytes, or an empty string when it can: a script only gets plain
/// data, whose bytes it may read and write in place.
[[nodiscard]] std::string whyNotPlainData(const ecs::TypeInfo& type, const bool mainThread) {
    if (mainThread) {
        return "it is main-thread only";
    }
    if (type.hooks.destruct != nullptr || type.hooks.move != nullptr || type.hooks.copy != nullptr) {
        return "it is not plain data (it owns memory of its own)";
    }
    return {};
}

/// @return The resources the system declared, checked against the world, with the access they ask of the scheduler.
[[nodiscard]] Result<std::pair<std::vector<ResourceTerm>, ecs::ResourceList>> makeResources(
    const ecs::Resources& resources, const SystemDeclaration& declaration) {
    std::pair<std::vector<ResourceTerm>, ecs::ResourceList> made;
    for (const auto& [name, write] : declaration.resources) {
        const ecs::ResourceInfo* info = resources.getRegistry().findByName(name);
        if (info == nullptr) {
            return fail(ErrorKind::kInvalidDeclaration,
                        std::format("System '{}': unknown resource '{}'", declaration.name, name));
        }
        if (const auto why = whyNotPlainData(info->type, info->nonSend); !why.empty()) {
            return fail(
                ErrorKind::kInvalidDeclaration,
                std::format("System '{}': resource '{}' cannot be used by a script: {}", declaration.name, name, why));
        }
        made.first.push_back({.type = &info->type, .write = write});
        made.second.resources.push_back({.id = info->type.id, .write = write});
    }
    return made;
}

/// @return The event types the system declared, checked against the world, with the access they ask of the scheduler.
[[nodiscard]] Result<std::pair<std::vector<EventTerm>, ecs::EventList>> makeEvents(
    const ecs::Events& events, const SystemDeclaration& declaration) {
    std::pair<std::vector<EventTerm>, ecs::EventList> made;
    for (const auto& [name, read, send] : declaration.events) {
        const ecs::EventInfo* info = events.getRegistry().findByName(name);
        if (info == nullptr) {
            return fail(ErrorKind::kInvalidDeclaration,
                        std::format("System '{}': unknown event '{}'", declaration.name, name));
        }
        if (const auto why = whyNotPlainData(info->type, false); !why.empty()) {
            return fail(
                ErrorKind::kInvalidDeclaration,
                std::format("System '{}': event '{}' cannot be used by a script: {}", declaration.name, name, why));
        }
        made.first.push_back({.type = &info->type, .read = read, .send = send});
        made.second.events.push_back({.id = info->type.id, .read = read, .send = send});
    }
    return made;
}

/// @brief Warns about a system of both sides that writes a server-only component: clients have none of it, so the
/// system finds nothing to write there.
void warnAboutServerOnlyWrites(const ecs::Components& components, const SystemDeclaration& declaration) {
    if (declaration.side != ecs::Side::kBoth) {
        return;
    }
    for (const auto& [kind, component] : declaration.terms) {
        const ecs::ComponentInfo* info = components.getRegistry().findByName(component);
        if (ecs::isWriteTerm(kind) && info != nullptr && info->type.presence == ecs::Presence::kServerOnly) {
            std::cerr << std::format(
                "[Scripting] System '{}' runs on both sides but writes '{}', which is server-only: it cannot write "
                "it on clients\n",
                declaration.name, component);
        }
    }
}
}  // namespace

Result<void> registerSystem(ecs::App& app, SystemDeclaration declaration, std::shared_ptr<RuntimeHandle> runtime) {
    auto& components = app.getWorld().getComponents();
    auto query = makeQuery(components, declaration);
    if (!query) {
        return tl::unexpected{std::move(query.error())};
    }
    auto resources = makeResources(app.getWorld().getResources(), declaration);
    if (!resources) {
        return tl::unexpected{std::move(resources.error())};
    }
    auto events = makeEvents(app.getWorld().getEvents(), declaration);
    if (!events) {
        return tl::unexpected{std::move(events.error())};
    }
    auto system = std::make_unique<ScriptSystem>(ScriptSystem{
        .name = declaration.name,
        .run = std::move(declaration.run),
        .terms = makeDataTerms(components, declaration),
        .resources = std::move(resources->first),
        .events = std::move(events->first),
        .clientSide = declaration.side == ecs::Side::kClient,
        .runtime = std::move(runtime),
        .scope = std::make_shared<view::ViewScope>(),
    });
    // The body keeps what a run needs; the scope of the views is shared, so commands a script kept can tell the system
    // is gone.
    ecs::ScriptCallable callable{
        [system = std::move(system)](const ecs::DynamicQueries queries, ecs::DynamicResources resources,
                                     const ecs::DynamicEvents events, const ecs::Res<ecs::Time> time,
                                     ecs::Commands& commands) {
            runScriptSystem(*system, queries, resources, events, time.get(), commands);
        },
    };
    warnAboutServerOnlyWrites(components, declaration);
    try {
        auto config =
            app.addSystem(declaration.phase, declaration.name, std::move(callable), ecs::QueryList{std::move(*query)},
                          std::move(resources->second), std::move(events->second));
        if (declaration.side) {
            config.setSide(*declaration.side);
        }
        for (auto& name : declaration.after) {
            config.after(std::move(name));
        }
        for (auto& name : declaration.before) {
            config.before(std::move(name));
        }
    } catch (const ecs::exceptions::ScheduleException& error) {
        return fail(ErrorKind::kRegistration, error.what());
    }
    return {};
}
}  // namespace rtype::scripting::system
