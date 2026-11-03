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

#include "SystemDeclaration.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/exceptions/ScheduleException.hpp"
#include "rtype/ecs/query/QueryBuilder.hpp"
#include "rtype/ecs/query/QueryTerm.hpp"
#include "rtype/ecs/system/DynamicQueries.hpp"
#include "rtype/ecs/system/QueryList.hpp"
#include "rtype/ecs/system/ScriptCallable.hpp"
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
}  // namespace

Result<void> registerSystem(ecs::App& app, SystemDeclaration declaration) {
    auto query = makeQuery(app.getWorld().getComponents(), declaration);
    if (!query) {
        return tl::unexpected{std::move(query.error())};
    }
    // The body only holds what LUA-06 calls with: the function and the scope of the views it hands out.
    ecs::ScriptCallable callable{
        [run = std::move(declaration.run), scope = std::make_unique<view::ViewScope>()](ecs::DynamicQueries /*queries*/,
                                                                                        ecs::Commands& /*commands*/) {
            static_cast<void>(run);
            static_cast<void>(scope);
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
