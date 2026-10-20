/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Script systems: what the Luau bridge and the network build on. A DynamicQueries parameter runs queries chosen when
// the system is added (by component id), with their access declared; a ScriptCallable is the one way to give a
// system a callable that keeps state: here a stand-in for a handle to a script function.

#include <chrono>
#include <cstddef>
#include <cstring>
#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/QueryBuilder.hpp"
#include "rtype/ecs/query/QueryMatch.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/DynamicQueries.hpp"
#include "rtype/ecs/system/QueryList.hpp"
#include "rtype/ecs/system/ScriptCallable.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/ecs/type/Field.hpp"

using namespace rtype::ecs;

namespace {
/// @return The float stored at `offset` in `bytes`.
float readFloat(std::span<const std::byte> bytes, std::size_t offset) {
    float value = 0.F;
    std::memcpy(&value, bytes.subspan(offset, sizeof(float)).data(), sizeof(float));
    return value;
}

/// @brief Stores `value` at `offset` in `bytes`.
void writeFloat(std::span<std::byte> bytes, std::size_t offset, float value) {
    std::memcpy(bytes.subspan(offset, sizeof(float)).data(), &value, sizeof(float));
}

/// @brief Stands for a handle to a script function: the real bridge would hold a reference into the Luau VM.
class ScriptHandle {
  public:
    explicit ScriptHandle(std::string name) : _name{std::move(name)}, _calls{std::make_shared<int>(0)} {}

    /// @brief "Runs the script": regenerates every shield the query gives (strength += regen).
    void call(DynamicQueries queries) const {
        ++*_calls;
        for (QueryMatch& match : queries.iter(0)) {
            const std::span<std::byte> shield = match.getMut(0);
            writeFloat(shield, 0, readFloat(shield, 0) + readFloat(shield, sizeof(float)));
            std::cout << std::format("    {}: #{} shield now {}\n", _name, match.getEntity().getIndex(),
                                     readFloat(shield, 0));
        }
    }

    /// @return How many times the script ran.
    [[nodiscard]] int getCalls() const { return *_calls; }

  private:
    std::string _name;            ///< The script's name.
    std::shared_ptr<int> _calls;  ///< Shared with the copy the system keeps, to read the count from outside.
};

/// @brief A plain C++ system using DynamicQueries, as the network's snapshot does: it reads components by id.
void snapshot(DynamicQueries replicated) {
    for (std::size_t query = 0; query < replicated.getCount(); ++query) {
        std::size_t count = 0;
        for (const QueryMatch& match : replicated.iter(query)) {
            static_cast<void>(match);
            ++count;
        }
        std::cout << std::format("    snapshot: query {} matches {} entities\n", query, count);
    }
}

/// @brief Runs the example, printing each step.
void run() {
    App app;
    app.addPlugin(TimePlugin::manual(std::chrono::milliseconds{20}));  // one fixed tick per frame
    Components& components = app.getWorld().getComponents();

    // A component a script declared: no C++ type, only a description.
    const ComponentId shield = *components.registerType(
        ComponentInfo{.type = {.name = "game.Shield",
                               .size = sizeof(float) * 2,
                               .alignment = alignof(float),
                               .fields = {{.name = "strength", .type = FieldType::kF32, .offset = 0},
                                          {.name = "regen", .type = FieldType::kF32, .offset = sizeof(float)}}}});
    for (const float regen : {1.F, 5.F}) {
        std::vector<std::byte> value(sizeof(float) * 2);
        writeFloat(value, 0, 10.F);
        writeFloat(value, sizeof(float), regen);
        components.add(app.getWorld().spawn(), shield, value);
    }

    std::cout << "== A script system: a ScriptCallable configured with a QueryList ==\n";
    const ScriptHandle script{"game.ShieldRegen"};
    app.addSystem(Phase::kFixedUpdate, "game.ShieldRegen",
                  ScriptCallable{[script](DynamicQueries queries) { script.call(queries); }},
                  QueryList{{QueryBuilder{}.write(shield)}});

    std::cout << "== A C++ system reading components by id, as replication does ==\n";
    app.addSystem(Phase::kPostUpdate, "net.snapshot", &snapshot, QueryList{{QueryBuilder{}.read(shield)}});

    for (int frame = 1; frame <= 2; ++frame) {
        std::cout << std::format("  frame {}:\n", frame);
        app.runFrame();
    }
    std::cout << std::format("the script ran {} times\n", script.getCalls());
}
}  // namespace

int main() {
    try {
        run();
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
