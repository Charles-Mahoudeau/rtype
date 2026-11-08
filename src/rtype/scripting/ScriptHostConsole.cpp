/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptHostConsole
*/

// The debug console of the host: ScriptHost::evaluate() and ScriptHost::complete(). The code is in Luau for what
// scripts show (console/ConsolePrelude.luau), and here for what only the host can do: finding the modules, and making
// their frozen tables writable for the time of one line.

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <set>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>
#include <vector>

#include "ScriptHost.hpp"
#include "console/ConsolePrelude.hpp"
#include "rtype/luau/ErrorKind.hpp"
#include "rtype/luau/Failure.hpp"
#include "rtype/luau/Function.hpp"
#include "rtype/luau/Result.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"

namespace rtype::scripting {

namespace {
constexpr std::size_t kMaxBindings = 150;  ///< Luau allows 200 locals in a function: leave room for the others.
constexpr std::size_t kMaxTables = 2000;   ///< How many tables the unfreezing visits at most.
constexpr std::string_view kBindingsName = "__b";
constexpr std::string_view kFormatName = "__format";

/// @brief Names a line cannot have for a module: the ones it declares itself, and the keywords.
constexpr auto kReserved = std::to_array<std::string_view>({
    "__b",    "__format", "engine", "modules",  "and",   "break", "continue", "do",   "else",   "elseif",
    "end",    "false",    "for",    "function", "if",    "in",    "local",    "nil",  "not",    "or",
    "repeat", "return",   "then",   "true",     "until", "while", "export",   "type", "typeof",
});

/// @return @p text as an identifier: anything else than a letter, a digit or `_` becomes `_`.
std::string identifierOf(const std::string_view text) {
    std::string name;
    for (const char letter : text) {
        const bool word = (letter >= 'a' && letter <= 'z') || (letter >= 'A' && letter <= 'Z') ||
                          (letter >= '0' && letter <= '9') || letter == '_';
        name += word ? letter : '_';
    }
    if (name.empty() || (name.front() >= '0' && name.front() <= '9')) {
        name.insert(name.begin(), '_');
    }
    return name;
}

/// @return The module @p key without its extension (`shared/config.luau` is `shared/config`): how a console says it.
std::string_view withoutExtension(std::string_view key) {
    for (const std::string_view extension : {".luau", ".lua"}) {
        if (key.ends_with(extension)) {
            return key.substr(0, key.size() - extension.size());
        }
    }
    return key;
}

/// @return The short name of the module @p key: its file name, or its folder's for an `init`.
std::string shortNameOf(const std::string_view key) {
    std::string_view rest = key;
    const std::size_t last = rest.rfind('/');
    std::string_view file = last == std::string_view::npos ? rest : rest.substr(last + 1);
    if (file == "init" && last != std::string_view::npos) {
        rest = rest.substr(0, last);
        const std::size_t before = rest.rfind('/');
        file = before == std::string_view::npos ? rest : rest.substr(before + 1);
    }
    return identifierOf(file);
}

/// @brief Appends to @p found the tables reachable from @p root that it does not hold yet, @p root included, up to
/// kMaxTables in all.
void collectTables(const luau::Table& root, std::vector<luau::Table>& found) {
    const auto known = [&found](const luau::Table& table) {
        return std::ranges::any_of(found, [&table](const luau::Table& other) {
            const auto same = other.refersToSame(table);
            return same && *same;
        });
    };
    std::vector<luau::Table> pending{root};
    while (!pending.empty() && found.size() < kMaxTables) {
        luau::Table table = std::move(pending.back());
        pending.pop_back();
        if (known(table)) {
            continue;
        }
        found.push_back(table);
        const auto entries = table.entries();
        if (!entries) {
            continue;
        }
        for (const auto& entry : *entries) {
            if (const auto* child = entry.second.tryAs<luau::Table>()) {
                pending.push_back(*child);
            }
        }
    }
}

/// @return The code of a line: what it declares (the bindings, then each name), then @p body.
std::string lineOf(const std::vector<std::string>& names, const std::string_view body, const bool isExpression) {
    std::string declared;
    std::string values;
    for (const std::string& name : names) {
        declared += declared.empty() ? "" : ", ";
        declared += name;
        values += values.empty() ? "" : ", ";
        values += std::format("{}.{}", kBindingsName, name);
    }
    // One line, so an error reads `console:1:` whatever the line is.
    return std::format("local {}, {} = ...; local {} = {}; {}{}{}", kBindingsName, kFormatName, declared, values,
                       isExpression ? std::format("return {}(", kFormatName) : "", body, isExpression ? ")" : "");
}
}  // namespace

luau::Result<luau::Table> ScriptHost::consoleTools() {
    if (_consoleTools) {
        return *_consoleTools;
    }
    if (!_sealed || !_loader) {
        return luau::Failure{luau::ErrorKind::kRuntime, "no script has run yet"};
    }
    auto script = _runtime.load(std::string{console::kConsolePreludeName}, std::string{console::getConsolePrelude()});
    if (!script) {
        return tl::unexpected{std::move(script.error())};
    }
    auto returned = _runtime.call(*script, {});
    if (!returned) {
        return tl::unexpected{std::move(returned.error())};
    }
    if (returned->size() != 1 || returned->values().front().tryAs<luau::Table>() == nullptr) {
        return luau::Failure{luau::ErrorKind::kRuntime, "the console's helpers did not return a table"};
    }
    _consoleTools = *returned->values().front().tryAs<luau::Table>();
    return *_consoleTools;
}

luau::Result<ScriptHost::ConsoleBindings> ScriptHost::consoleBindings(const luau::Table& tools) {
    auto table = luau::Table::create(_runtime);
    auto modules = luau::Table::create(_runtime);
    if (!table || !modules) {
        return luau::Failure{luau::ErrorKind::kOutOfMemory, "cannot make the console's tables"};
    }
    ConsoleBindings bindings{.table = *table, .names = {}, .moduleTables = {}};
    std::set<std::string> taken(kReserved.begin(), kReserved.end());
    for (const std::string& key : _loader->loadedKeys()) {
        auto value = _loader->valueOf(key);
        if (!value || value->is<luau::Value::Nil>()) {
            continue;
        }
        const std::string shown{withoutExtension(key)};
        if (auto stored = modules->set(shown, *value); !stored) {
            return tl::unexpected{std::move(stored.error())};
        }
        if (const auto* moduleTable = value->tryAs<luau::Table>()) {
            bindings.moduleTables.push_back(*moduleTable);
        }
        // The file name if it is free, else the whole key; a module that has neither is reached through `modules`.
        std::string name = shortNameOf(shown);
        if (taken.contains(name)) {
            name = identifierOf(shown);
        }
        if (taken.contains(name) || bindings.names.size() >= kMaxBindings) {
            continue;
        }
        taken.insert(name);
        bindings.names.push_back(name);
        if (auto stored = bindings.table.set(name, *value); !stored) {
            return tl::unexpected{std::move(stored.error())};
        }
    }
    auto engineView = tools.function("engineView");
    if (!engineView) {
        return tl::unexpected{std::move(engineView.error())};
    }
    luau::ValuePack arguments;
    arguments.add(luau::Value{_engine->table()});
    auto view = engineView->call(arguments);
    if (!view || view->size() != 1) {
        return luau::Failure{luau::ErrorKind::kRuntime, "cannot make the console's `engine`"};
    }
    const auto bind = [&bindings](const std::string& name, const luau::Value& value) -> luau::Result<> {
        if (auto stored = bindings.table.set(name, value); !stored) {
            return tl::unexpected{std::move(stored.error())};
        }
        bindings.names.push_back(name);
        return {};
    };
    if (auto bound = bind("modules", luau::Value{*modules}); !bound) {
        return tl::unexpected{std::move(bound.error())};
    }
    if (auto bound = bind("engine", view->values().front()); !bound) {
        return tl::unexpected{std::move(bound.error())};
    }
    return bindings;
}

ScriptHost::Evaluation ScriptHost::evaluate(const std::string_view source) {
    const auto failure = [](const std::string_view message) {
        return Evaluation{.ok = false, .text = std::string{message}};
    };
    auto tools = consoleTools();
    if (!tools) {
        return failure(tools.error().message());
    }
    auto bindings = consoleBindings(*tools);
    if (!bindings) {
        return failure(bindings.error().message());
    }
    // As an expression first (its values are written); if that is not Luau, as a statement.
    auto script = _runtime.load("console", lineOf(bindings->names, source, true));
    const bool isExpression = script.has_value();
    if (!script) {
        script = _runtime.load("console", lineOf(bindings->names, source, false));
    }
    if (!script) {
        return failure(script.error().message());
    }
    auto format = tools->get("format");
    if (!format) {
        return failure(format.error().message());
    }
    luau::ValuePack arguments;
    arguments.add(luau::Value{bindings->table});
    arguments.add(*format);

    // The modules return frozen tables: the line may change them, then they are frozen as they were. `engine` and the
    // console's own tables are not the game's data, they stay as they are.
    std::vector<luau::Table> protectedTables;
    collectTables(_engine->table(), protectedTables);
    collectTables(*tools, protectedTables);
    const std::size_t protectedCount = protectedTables.size();
    for (const luau::Table& module : bindings->moduleTables) {
        collectTables(module, protectedTables);
    }
    std::vector<luau::Table> unfrozen;
    for (std::size_t index = protectedCount; index < protectedTables.size(); ++index) {
        luau::Table& table = protectedTables.at(index);
        if (const auto frozen = table.isFrozen(); frozen && *frozen && table.unfreeze()) {
            unfrozen.push_back(table);
        }
    }
    auto returned = _runtime.call(*script, arguments);
    for (luau::Table& table : unfrozen) {
        std::ignore = table.freeze();
    }

    if (!returned) {
        return failure(returned.error().message());
    }
    std::string text;
    if (isExpression && returned->size() == 1) {
        if (const auto* written = returned->values().front().tryAs<std::string>()) {
            text = *written;
        }
    }
    return Evaluation{.ok = true, .text = std::move(text)};
}

std::vector<std::string> ScriptHost::complete(const std::string_view word) {
    auto tools = consoleTools();
    if (!tools) {
        return {};
    }
    auto bindings = consoleBindings(*tools);
    if (!bindings) {
        return {};
    }
    luau::ValuePack arguments;
    arguments.add(luau::Value{bindings->table});
    arguments.add(luau::Value{std::string{word}});
    auto function = tools->function("complete");
    if (!function) {
        return {};
    }
    const auto returned = function->call(arguments);
    if (!returned || returned->size() != 1) {
        return {};
    }
    const auto* list = returned->values().front().tryAs<luau::Table>();
    if (list == nullptr) {
        return {};
    }
    std::vector<std::string> words;
    const auto entries = list->entries();
    if (!entries) {
        return {};
    }
    for (const auto& entry : *entries) {
        if (const auto* candidate = entry.second.tryAs<std::string>()) {
            words.push_back(*candidate);
        }
    }
    std::ranges::sort(words);
    return words;
}

}  // namespace rtype::scripting
