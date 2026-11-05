/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SystemRegistration
*/

#include "SystemRegistration.hpp"

#include <format>
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
#include "rtype/ecs/exceptions/ScheduleException.hpp"
#include "rtype/ecs/query/QueryBuilder.hpp"
#include "rtype/ecs/query/QueryTerm.hpp"
#include "rtype/ecs/system/DynamicQueries.hpp"
#include "rtype/ecs/system/QueryList.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/ScriptCallable.hpp"
#include "rtype/ecs/time/Time.hpp"
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

/// @return The builder of the system's query, checked against the world's components, or the reason it is refused.
[[nodiscard]] Result<ecs::QueryBuilder> makeQuery(ecs::Components& components, const SystemDeclaration& declaration) {
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
    return builder;
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
}  // namespace

Result<void> registerSystem(ecs::App& app, SystemDeclaration declaration, std::shared_ptr<RuntimeHandle> runtime) {
    auto& components = app.getWorld().getComponents();
    auto query = makeQuery(components, declaration);
    if (!query) {
        return tl::unexpected{std::move(query.error())};
    }
    auto system = std::make_unique<ScriptSystem>(ScriptSystem{
        .name = declaration.name,
        .run = std::move(declaration.run),
        .terms = makeDataTerms(components, declaration),
        .runtime = std::move(runtime),
        .scope = std::make_shared<view::ViewScope>(),
    });
    // The body keeps what a run needs; the scope of the views is shared, so commands a script kept can tell the system
    // is gone.
    ecs::ScriptCallable callable{
        [system = std::move(system)](const ecs::DynamicQueries queries, const ecs::Res<ecs::Time> time,
                                     ecs::Commands& commands) {
            runScriptSystem(*system, queries, time.get(), commands);
        },
    };
    try {
        auto config = app.addSystem(declaration.phase, declaration.name, std::move(callable),
                                    ecs::QueryList{{std::move(*query)}});
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
