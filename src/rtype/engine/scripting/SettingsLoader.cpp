/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SettingsLoader
*/

#include "SettingsLoader.hpp"

#include <cstddef>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/core/config/Settings.hpp"
#include "rtype/engine/Exceptions.hpp"
#include "rtype/luau/Function.hpp"
#include "rtype/luau/Result.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/RuntimeConfig.hpp"
#include "rtype/luau/Script.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"

namespace {
/// @return A runtime with no standard library and no optimization: project scripts only assign settings.
rtype::luau::Runtime createRuntime() {
    constexpr rtype::luau::RuntimeConfig config{
        .libs = rtype::luau::RuntimeConfig::Libs::kNone,
        .optimizationLevel = rtype::luau::RuntimeConfig::OptimizationLevel::kDisabled,
    };
    return rtype::luau::expect(rtype::luau::Runtime::create(config));
}

/// @return A new empty table of the runtime's state.
rtype::luau::Table createTable(const rtype::luau::Runtime& runtime) {
    return rtype::luau::expect(rtype::luau::Table::create(runtime.state()));
}

/// @throws rtype::engine::exceptions::RuntimeError Always, naming the offending setting.
[[noreturn]] void fail(const std::string_view key, const std::string_view reason) {
    throw rtype::engine::exceptions::RuntimeError{std::format("Invalid setting '{}': {}", key, reason)};
}

void addTable(rtype::core::config::Settings& settings, const rtype::luau::Table& table, const std::string& prefix);

/// Reads the `size` elements of a Luau array, which must all hold a `T`.
template <typename T>
std::vector<T> readList(const rtype::luau::Table& table, const std::size_t size, const std::string& key) {
    std::vector<T> list;

    list.reserve(size);
    for (std::size_t index = 1; index <= size; ++index) {
        const rtype::luau::Value element = rtype::luau::expect(table.getAt(index));
        const T* item = element.tryAs<T>();

        if (!item) {
            fail(key, std::format("element {} does not have the type of the first one", index));
        }
        list.push_back(*item);
    }
    return list;
}

/// Stores a Luau array as a list of numbers or of strings, according to its first element.
void addList(rtype::core::config::Settings& settings, const rtype::luau::Table& table, const std::size_t size,
             const std::string& key) {
    if (rtype::luau::expect(table.count()) != size) {
        fail(key, "an array cannot have named keys or holes");
    }
    if (const rtype::luau::Value first = rtype::luau::expect(table.getAt(1)); first.is<double>()) {
        settings.set(key, readList<double>(table, size, key));
    } else if (first.is<std::string>()) {
        settings.set(key, readList<std::string>(table, size, key));
    } else {
        fail(key, "an array can only hold numbers or strings");
    }
}

/// Stores one value under `key`: a scalar, an array, or the flattened entries of a table.
// NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the settings table.
void addValue(rtype::core::config::Settings& settings, const rtype::luau::Value& value, const std::string& key) {
    if (const auto* flag = value.tryAs<bool>()) {
        settings.set(key, *flag);
    } else if (const auto* number = value.tryAs<double>()) {
        settings.set(key, *number);
    } else if (const auto* text = value.tryAs<std::string>()) {
        settings.set(key, *text);
    } else if (const auto* table = value.tryAs<rtype::luau::Table>()) {
        if (const std::size_t size = rtype::luau::expect(table->size()); size > 0) {
            addList(settings, *table, size, key);
        } else if (rtype::luau::expect(table->count()) > 0) {
            addTable(settings, *table, key);
        }
        // An empty table is an empty group (one a script only read): nothing to store.
    } else {
        fail(key, "a setting can only be a boolean, a number, a string, an array or a table");
    }
}

/// Flattens the named entries of a table into dotted keys under `prefix`.
// NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the settings table.
void addTable(rtype::core::config::Settings& settings, const rtype::luau::Table& table, const std::string& prefix) {
    for (const auto& [name, value] : rtype::luau::expect(table.entries())) {
        const auto* part = name.tryAs<std::string>();

        if (part == nullptr) {
            fail(prefix.empty() ? "settings" : prefix,
                 "keys must be strings (an array cannot be mixed with named keys)");
        }
        addValue(settings, value, prefix.empty() ? *part : std::format("{}.{}", prefix, *part));
    }
}
}  // namespace

namespace rtype::engine::scripting {
SettingsLoader::SettingsLoader()
    : _runtime{createRuntime()},
      _engineTable{createTable(_runtime)},
      _settingsTable{createTable(_runtime)},
      _groupMetatable{createTable(_runtime)},
      _createGroup{luau::Function::create(_runtime.state(), "settings.__index")} {
    // `__index(group, key)` runs when a script reads a key a group does not have: it stores an empty group under it
    // and returns it, with the same metatable, so that `a.b.c = 1` works at any depth.
    _createGroup.bind(
        [state = _runtime.state(), metatable = _groupMetatable](const luau::Table& group, const std::string& key) {
            luau::Table parent = group;
            luau::Table child = luau::expect(luau::Table::create(state));
            luau::ValuePack result;

            luau::expect(child.setMetatable(metatable));
            luau::expect(parent.set(key, luau::Value{child}));
            result.add(luau::Value{child});
            return result;
        });
    luau::expect(_groupMetatable.set("__index", luau::Value{_createGroup.ref()}));
    luau::expect(_settingsTable.setMetatable(_groupMetatable));
    luau::expect(_engineTable.set("settings", {_settingsTable}));
    luau::expect(_runtime.globals().set("engine", luau::Value{_engineTable}));
}

// ReSharper disable once CppMemberFunctionMayBeConst
core::config::Settings SettingsLoader::build() {
    core::config::Settings settings;

    addTable(settings, _settingsTable, "");
    return settings;
}

void SettingsLoader::setFlag(const std::string_view name, bool enabled) {
    luau::expect(_engineTable.set(name, {enabled}));
}

// ReSharper disable once CppMemberFunctionMayBeConst
void SettingsLoader::load(const std::filesystem::path& path) {
    const luau::Script script = rtype::luau::expect(_runtime.load(path));

    luau::expect(_runtime.run(script));
}
}  // namespace rtype::engine::scripting
