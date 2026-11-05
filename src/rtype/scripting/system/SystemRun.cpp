/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SystemRun
*/

#include "SystemRun.hpp"

#include <cstddef>
#include <format>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>

#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/QueryIteration.hpp"
#include "rtype/ecs/query/QueryMatch.hpp"
#include "rtype/ecs/system/DynamicQueries.hpp"
#include "rtype/ecs/time/Time.hpp"
#include "rtype/luau/CallContext.hpp"
#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/Function.hpp"
#include "rtype/luau/Result.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Userdata.hpp"
#include "rtype/luau/Usertype.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"
#include "rtype/scripting/commands/ScriptCommands.hpp"
#include "rtype/scripting/view/ComponentView.hpp"

namespace rtype::scripting::system {
namespace {
/// @return The value a script gets for the user data it was given, or the error that made it.
[[nodiscard]] luau::Value handOver(luau::Result<luau::Userdata> userdata) {
    if (!userdata) {
        throw luau::exceptions::RuntimeError{std::string{userdata.error().message()}};
    }
    return luau::Value{std::move(*userdata)};
}

/// @brief What the iterator of `ctx.query` steps: the pass over the matches and the views handed out for each.
class QueryStepper {
  public:
    QueryStepper(ScriptSystem& system, const luau::Runtime& runtime, ecs::QueryIteration iteration)
        : _system{&system},
          _entities{runtime.usertype<ecs::Entity>()},
          _views{runtime.usertype<view::ComponentView>()},
          _iteration{std::move(iteration)},
          _current{_iteration.begin()} {}
    ~QueryStepper() = default;
    // The iterator points into _iteration, so a stepper stays where it is built.
    QueryStepper(const QueryStepper& other) = delete;
    QueryStepper& operator=(const QueryStepper& other) = delete;
    QueryStepper(QueryStepper&& other) = delete;
    QueryStepper& operator=(QueryStepper&& other) = delete;

    /// @return The entity and views of the next match, or an empty pack when there is none (which ends a `for`).
    luau::ValuePack next() {
        if (_done) {
            return {};
        }
        if (_started) {
            ++_current;
        }
        _started = true;
        if (_current == ecs::QueryIteration::end()) {
            _done = true;
            return {};
        }
        ecs::QueryMatch& match = *_current;
        luau::ValuePack pack;
        pack.add(handOver(_entities.create(match.getEntity())));
        for (std::size_t index = 0; index < _system->terms.size(); ++index) {
            pack.add(makeView(match, index));
        }
        return pack;
    }

  private:
    /// @return The view of data term `index` of the match, or `nil` when the entity has no such optional component.
    [[nodiscard]] luau::Value makeView(ecs::QueryMatch& match, const std::size_t index) const {
        const auto& [info, readOnly] = _system->terms.at(index);
        std::span<std::byte> bytes;
        if (readOnly) {
            const std::span<const std::byte> read = match.get(index);
            // The view refuses writes, so the bytes are never written through it.
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            bytes = {const_cast<std::byte*>(read.data()), read.size()};
        } else {
            bytes = match.getMut(index);
        }
        if (bytes.empty()) {
            return luau::Value{luau::Value::Nil{}};
        }
        return handOver(_views.create(view::ComponentView{
            .info = info,
            .bytes = bytes,
            .scope = _system->scope.get(),
            .generation = _system->scope->generation,
            .readOnly = readOnly,
        }));
    }

    ScriptSystem* _system;                       ///< The system being run.
    luau::Usertype<ecs::Entity> _entities;       ///< Makes the `Entity` of each match.
    luau::Usertype<view::ComponentView> _views;  ///< Makes the views of each match.
    ecs::QueryIteration _iteration;              ///< The pass over the query.
    ecs::QueryIteration::Iterator _current;      ///< The current match of the pass.
    bool _started{false};                        ///< Whether the first match was handed out.
    bool _done{false};                           ///< Whether the pass reached its end.
};

/// @brief Builds `ctx` and calls `run` with it.
[[nodiscard]] luau::Result<> call(ScriptSystem& system, const luau::Runtime& runtime,
                                  const ecs::DynamicQueries& queries, const ecs::Time& time, ecs::Commands& commands) {
    // The iterator is a native function: it must stay alive while `run` can call it.
    QueryStepper stepper{system, runtime, queries.iter(0)};
    auto iterator = luau::Function::create(runtime, std::format("{}.query", system.name));
    iterator.setHandler([&stepper](const luau::CallContext& /*arguments*/) { return stepper.next(); });

    auto recorder = runtime.usertype<commands::ScriptCommands>().create(commands::ScriptCommands{
        .commands = &commands,
        .layouts = system.runtime->components,
        .scope = system.scope,
        .generation = system.scope->generation,
    });
    if (!recorder) {
        return tl::unexpected{std::move(recorder.error())};
    }

    auto ctx = luau::Table::create(runtime, 0, 4);
    if (!ctx) {
        return tl::unexpected{std::move(ctx.error())};
    }
    for (const auto& [name, value] : {
             std::pair<std::string_view, luau::Value>{"query", luau::Value{iterator.ref()}},
             {"delta", luau::Value{static_cast<double>(time.delta)}},
             {"fixedDelta", luau::Value{static_cast<double>(time.fixedDelta)}},
             {"commands", luau::Value{std::move(*recorder)}},
         }) {
        if (auto set = ctx->set(name, value); !set) {
            return set;
        }
    }
    luau::ValuePack arguments;
    arguments.add(luau::Value{*ctx});
    auto results = luau::Function::fromRef(system.run, system.name).call(arguments);
    if (!results) {
        return tl::unexpected{std::move(results.error())};
    }
    return {};
}
}  // namespace

void runScriptSystem(ScriptSystem& system, const ecs::DynamicQueries& queries, const ecs::Time& time,
                     ecs::Commands& commands) {
    const luau::Runtime* const runtime = system.runtime->runtime;
    if (runtime == nullptr) {
        return;  // the host is gone, and the script with it
    }
    const auto result = call(system, *runtime, queries, time, commands);
    ++system.scope->generation;  // every view of this run dies, even when it failed
    if (!result) {
        std::cerr << std::format("[ScriptSystem] '{}' failed, skipped this tick: {}\n", system.name,
                                 result.error().message());
    }
}
}  // namespace rtype::scripting::system
