/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SystemRun
*/

#include "SystemRun.hpp"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <format>
#include <iostream>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>
#include <vector>

#include "ContextTypes.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/event/EventRange.hpp"
#include "rtype/ecs/query/QueryIteration.hpp"
#include "rtype/ecs/query/QueryMatch.hpp"
#include "rtype/ecs/system/DynamicEvents.hpp"
#include "rtype/ecs/system/DynamicQueries.hpp"
#include "rtype/ecs/system/DynamicResources.hpp"
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
#include "rtype/scripting/view/FieldAccess.hpp"

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
            .type = &info->type,
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

/// @brief What `ctx.events` gives a run: the declared events, the iterators it handed out, and what they need.
class EventsContext {
  public:
    EventsContext(ScriptSystem& system, const luau::Runtime& runtime, const ecs::DynamicEvents& events)
        : _system{&system}, _runtime{&runtime}, _events{events}, _views{runtime.usertype<view::ComponentView>()} {}

    ~EventsContext() = default;
    // The functions it handed out point to it, so it stays where it is built.
    EventsContext(const EventsContext& other) = delete;
    EventsContext& operator=(const EventsContext& other) = delete;
    EventsContext(EventsContext&& other) = delete;
    EventsContext& operator=(EventsContext&& other) = delete;

    /// @brief Builds `ctx.events`: a table with `send` and `read`.
    [[nodiscard]] luau::Result<luau::Table> makeTable() {
        auto table = luau::Table::create(*_runtime, 0, 2);
        if (!table) {
            return table;
        }
        auto& send =
            _functions.emplace_back(luau::Function::create(*_runtime, std::format("{}.events.send", _system->name)));
        send.setHandler([this](const luau::CallContext& arguments) { return this->send(arguments); });
        auto& read =
            _functions.emplace_back(luau::Function::create(*_runtime, std::format("{}.events.read", _system->name)));
        read.setHandler([this](const luau::CallContext& arguments) { return this->read(arguments); });
        if (auto set = table->set("send", luau::Value{send.ref()}); !set) {
            return tl::unexpected{std::move(set.error())};
        }
        if (auto set = table->set("read", luau::Value{read.ref()}); !set) {
            return tl::unexpected{std::move(set.error())};
        }
        return table;
    }

  private:
    /// @brief One pass over the events of a type: what the iterator of `ctx.events.read` steps.
    struct Pass {
        const ecs::TypeInfo* type;          ///< The event type, for its views.
        ecs::RawEventRange::Iterator next;  ///< The next event to hand out.
        ecs::RawEventRange::Iterator end;   ///< One past the last event.
    };

    /// @return The index and term of the declared event called `name`, or a Luau error.
    [[nodiscard]] std::pair<std::size_t, const EventTerm&> find(const luau::CallContext& arguments,
                                                                const std::string_view function) const {
        const auto* name = arguments.argCount() > 0 ? arguments.args().front().tryAs<std::string>() : nullptr;
        if (name == nullptr) {
            throw luau::exceptions::RuntimeError{std::format("events.{} expects the name of an event", function)};
        }
        const auto found =
            std::ranges::find(_system->events, *name, [](const EventTerm& term) { return term.type->name; });
        if (found == _system->events.end()) {
            throw luau::exceptions::RuntimeError{
                std::format("event '{}' is not declared by system '{}': list it under 'events'", *name, _system->name)};
        }
        return {static_cast<std::size_t>(std::distance(_system->events.begin(), found)), *found};
    }

    /// @brief `ctx.events.send(name, fields)`: sends an event with its defaults and the fields written over them.
    luau::ValuePack send(const luau::CallContext& arguments) {
        const auto [index, term] = find(arguments, "send");
        if (!term.send) {
            throw luau::exceptions::RuntimeError{
                std::format("event '{}' is declared to be read by system '{}', not sent: list it under 'events.send'",
                            term.type->name, _system->name)};
        }
        const luau::Table* fields = nullptr;
        if (arguments.argCount() > 1 && !arguments.args()[1].is<luau::Value::Nil>()) {
            fields = arguments.args()[1].tryAs<luau::Table>();
            if (fields == nullptr) {
                throw luau::exceptions::RuntimeError{"events.send expects a table of fields as its second argument"};
            }
        }
        std::vector<std::byte> bytes(term.type->size);
        if (term.type->hooks.construct != nullptr) {
            term.type->hooks.construct(bytes.data());  // the defaults the type's members declare
        }
        if (fields != nullptr) {
            if (auto filled = view::fillFromTable(*term.type, *fields, bytes); !filled) {
                throw luau::exceptions::RuntimeError{std::string{filled.error().message()}};
            }
        }
        if (!_events.send(index, bytes)) {
            throw luau::exceptions::RuntimeError{std::format("event '{}' cannot be sent", term.type->name)};
        }
        return {};
    }

    /// @brief `ctx.events.read(name)`: an iterator over the events of the type not read yet.
    luau::ValuePack read(const luau::CallContext& arguments) {
        const auto [index, term] = find(arguments, "read");
        if (!term.read) {
            throw luau::exceptions::RuntimeError{
                std::format("event '{}' is declared to be sent by system '{}', not read: list it under 'events.read'",
                            term.type->name, _system->name)};
        }
        const ecs::RawEventRange range = _events.read(index);
        auto& pass = _passes.emplace_back(Pass{.type = term.type, .next = range.begin(), .end = range.end()});
        auto& iterator =
            _functions.emplace_back(luau::Function::create(*_runtime, std::format("{}.read", term.type->name)));
        iterator.setHandler([this, &pass](const luau::CallContext& /*arguments*/) { return step(pass); });
        luau::ValuePack pack;
        pack.add(luau::Value{iterator.ref()});
        return pack;
    }

    /// @return A view of the next event of the pass, or an empty pack when there is none (which ends a `for`).
    luau::ValuePack step(Pass& pass) {
        if (pass.next == pass.end) {
            return {};
        }
        const std::span<const std::byte> read = *pass.next;
        ++pass.next;
        // The view refuses writes, so the bytes are never written through it.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        const std::span<std::byte> bytes{const_cast<std::byte*>(read.data()), read.size()};
        luau::ValuePack pack;
        pack.add(handOver(_views.create(view::ComponentView{
            .type = pass.type,
            .bytes = bytes,
            .scope = _system->scope.get(),
            .generation = _system->scope->generation,
            .readOnly = true,
        })));
        return pack;
    }

    ScriptSystem* _system;                       ///< The system being run.
    const luau::Runtime* _runtime;               ///< The runtime the functions are made in.
    ecs::DynamicEvents _events;                  ///< The events of the run.
    luau::Usertype<view::ComponentView> _views;  ///< Makes the views of the events read.
    std::deque<luau::Function> _functions;       ///< `send`, `read` and the iterators handed out; a deque keeps
                                                 ///< their addresses.
    std::deque<Pass> _passes;                    ///< What each iterator steps.
};

/// @brief Builds `ctx` and calls `run` with it.
[[nodiscard]] luau::Result<> call(ScriptSystem& system, const luau::Runtime& runtime,
                                  const ecs::DynamicQueries& queries, ecs::DynamicResources& resources,
                                  const ecs::DynamicEvents& events, const ecs::Time& time, ecs::Commands& commands) {
    // The iterator is a native function: it must stay alive while `run` can call it.
    // A system without query terms has no query: its iterator ends at once.
    std::optional<QueryStepper> stepper;
    if (queries.getCount() > 0) {
        stepper.emplace(system, runtime, queries.iter(0));
    }
    auto iterator = luau::Function::create(runtime, std::format("{}.query", system.name));
    iterator.setHandler(
        [&stepper](const luau::CallContext& /*arguments*/) { return stepper ? stepper->next() : luau::ValuePack{}; });

    auto recorder = runtime.usertype<commands::ScriptCommands>().create(commands::ScriptCommands{
        .commands = &commands,
        .layouts = system.runtime->components,
        .scope = system.scope,
        .generation = system.scope->generation,
    });
    if (!recorder) {
        return tl::unexpected{std::move(recorder.error())};
    }

    auto resourcesHandle = runtime.usertype<ResourcesHandle>().create(ResourcesHandle{
        .system = &system,
        .resources = &resources,
        .scope = system.scope,
        .generation = system.scope->generation,
    });
    if (!resourcesHandle) {
        return tl::unexpected{std::move(resourcesHandle.error())};
    }
    EventsContext eventsContext{system, runtime, events};
    auto eventsTable = eventsContext.makeTable();
    if (!eventsTable) {
        return tl::unexpected{std::move(eventsTable.error())};
    }
    auto ctx = luau::Table::create(runtime, 0, 6);
    if (!ctx) {
        return tl::unexpected{std::move(ctx.error())};
    }
    for (const auto& [name, value] : {
             std::pair<std::string_view, luau::Value>{"query", luau::Value{iterator.ref()}},
             {"delta", luau::Value{static_cast<double>(time.delta)}},
             {"fixedDelta", luau::Value{static_cast<double>(time.fixedDelta)}},
             {"commands", luau::Value{std::move(*recorder)}},
             {"resources", luau::Value{std::move(*resourcesHandle)}},
             {"events", luau::Value{*eventsTable}},
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

void runScriptSystem(ScriptSystem& system, const ecs::DynamicQueries& queries, ecs::DynamicResources& resources,
                     const ecs::DynamicEvents& events, const ecs::Time& time, ecs::Commands& commands) {
    const luau::Runtime* const runtime = system.runtime->runtime;
    if (runtime == nullptr) {
        return;  // the host is gone, and the script with it
    }
    const auto result = call(system, *runtime, queries, resources, events, time, commands);
    ++system.scope->generation;  // every view of this run dies, even when it failed
    if (!result) {
        std::cerr << std::format("[ScriptSystem] '{}' failed, skipped this tick: {}\n", system.name,
                                 result.error().message());
    }
}
}  // namespace rtype::scripting::system
