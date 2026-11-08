/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PersistenceCalls
*/

#include "plugins/persistence/PersistenceCalls.hpp"

#include <array>
#include <string>
#include <string_view>
#include <utility>

#include "plugins/persistence/SaveStore.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

namespace rtype::plugins::persistence {
namespace {
SaveOutcome toOutcome(SaveResult result) {
    return SaveOutcome{.ok = result.ok, .data = std::move(result.data), .error = std::move(result.error)};
}

SaveOutcome readSave(ecs::Res<SaveStore> store, const NameArguments& arguments) {
    return toOutcome(store->read(arguments.name));
}

SaveOutcome writeSave(ecs::Res<SaveStore> store, const WriteArguments& arguments) {
    return toOutcome(store->write(arguments.name, arguments.data));
}

SaveOutcome deleteSave(ecs::Res<SaveStore> store, const NameArguments& arguments) {
    return toOutcome(store->remove(arguments.name));
}

ExistsResult existsSave(ecs::Res<SaveStore> store, const NameArguments& arguments) {
    return ExistsResult{.exists = store->exists(arguments.name)};
}
}  // namespace

void addPersistenceForms(ecs::PluginRegistry& registry) {
    static constexpr auto kForms = std::to_array<char>({
#include "PersistenceForms.luau.h"
    });
    // bin2c ends the bytes with a null terminator, which is not part of the source.
    registry.addScriptForms("addPersistenceForms", std::string_view{kForms.data(), kForms.size() - 1});
}

void addPersistenceCalls(ecs::App& app) {
    addPersistenceForms(app.getPluginRegistry());
    app.addCall(std::string{kReadCall}, &readSave);
    app.addCall(std::string{kWriteCall}, &writeSave);
    app.addCall(std::string{kDeleteCall}, &deleteSave);
    app.addCall(std::string{kExistsCall}, &existsSave);
}

}  // namespace rtype::plugins::persistence
