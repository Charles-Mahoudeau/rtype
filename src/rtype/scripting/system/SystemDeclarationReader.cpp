/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SystemDeclarationReader
*/

#include "SystemDeclarationReader.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>
#include <vector>

#include "SystemDeclaration.hpp"
#include "rtype/ecs/query/QueryTerm.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/luau/FunctionRef.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/scripting/Error.hpp"
#include "rtype/scripting/ErrorKind.hpp"
#include "rtype/scripting/Result.hpp"

namespace rtype::scripting::system {
namespace {
struct TermName {
    std::string_view name;
    ecs::TermKind kind;
};

/// @brief The lists of `query`, in the order their terms come out.
constexpr std::array kTerms{
    TermName{.name = "read", .kind = ecs::TermKind::kRead},
    TermName{.name = "write", .kind = ecs::TermKind::kWrite},
    TermName{.name = "optionalRead", .kind = ecs::TermKind::kOptionalRead},
    TermName{.name = "optionalWrite", .kind = ecs::TermKind::kOptionalWrite},
    TermName{.name = "with", .kind = ecs::TermKind::kWith},
    TermName{.name = "without", .kind = ecs::TermKind::kWithout},
    TermName{.name = "added", .kind = ecs::TermKind::kAdded},
    TermName{.name = "changed", .kind = ecs::TermKind::kChanged},
};

constexpr std::array kPhases{
    ecs::Phase::kStartup, ecs::Phase::kPreUpdate,  ecs::Phase::kFixedUpdate,
    ecs::Phase::kUpdate,  ecs::Phase::kPostUpdate, ecs::Phase::kRender,
};

/// @brief What the reader works on: the system's name, for the messages.
struct Source {
    std::string_view system;
};

[[nodiscard]] tl::unexpected<Error> invalid(const std::string_view system, const std::string& what) {
    return fail(ErrorKind::kInvalidDeclaration, std::format("System '{}': {}", system, what));
}

/// @brief The entries of a table whose keys are all strings, sorted by key.
[[nodiscard]] Result<std::vector<luau::Table::Entry>> namedEntries(const std::string_view system,
                                                                   const luau::Table& table,
                                                                   const std::string_view what) {
    auto entries = table.entries();
    if (!entries) {
        return invalid(system, std::format("cannot read {}: {}", what, entries.error().message()));
    }
    for (const auto& [key, value] : *entries) {
        if (!key.is<std::string>()) {
            return invalid(system, std::format("the keys of {} must be strings", what));
        }
    }
    std::ranges::sort(*entries, [](const luau::Table::Entry& left, const luau::Table::Entry& right) {
        return left.first.as<std::string>() < right.first.as<std::string>();
    });
    return std::move(*entries);
}

/// @brief The strings of a list, in order: `{"game.Shield", "game.Armor"}`.
[[nodiscard]] Result<std::vector<std::string>> readNames(const std::string_view system, const luau::Value& value,
                                                         const std::string_view what) {
    const auto* table = value.tryAs<luau::Table>();
    if (table == nullptr) {
        return invalid(system, std::format("'{}' must be a list of strings", what));
    }
    const auto size = table->size();
    const auto count = table->count();
    if (!size || !count) {
        return invalid(system, std::format("cannot read '{}'", what));
    }
    if (*size != *count) {
        return invalid(system, std::format("'{}' must be a list of strings, without holes or named keys", what));
    }
    std::vector<std::string> names;
    for (std::size_t index = 1; index <= *size; ++index) {
        const auto item = table->getAt(index);
        const auto* name = item ? item->tryAs<std::string>() : nullptr;
        if (name == nullptr) {
            return invalid(system, std::format("'{}' must be a list of strings", what));
        }
        names.push_back(*name);
    }
    return names;
}

[[nodiscard]] Result<ecs::Phase> readPhase(const std::string_view system, const luau::Value& value) {
    const auto* name = value.tryAs<std::string>();
    if (name == nullptr) {
        return invalid(system, "'phase' must be a string");
    }
    for (const ecs::Phase phase : kPhases) {
        if (ecs::toString(phase) == *name) {
            return phase;
        }
    }
    return invalid(system, std::format("unknown phase '{}'", *name));
}

[[nodiscard]] Result<std::vector<TermDeclaration>> readQuery(const Source& source, const luau::Value& value) {
    const auto* table = value.tryAs<luau::Table>();
    if (table == nullptr) {
        return invalid(source.system, "'query' must be a table");
    }
    const auto entries = namedEntries(source.system, *table, "'query'");
    if (!entries) {
        return tl::unexpected{entries.error()};
    }
    for (const auto& [key, list] : *entries) {
        const auto& option = key.as<std::string>();
        if (std::ranges::find(kTerms, option, &TermName::name) != kTerms.end()) {
            continue;
        }
        return invalid(source.system, std::format("unknown query term '{}' (expected 'read', 'write', 'optionalRead', "
                                                  "'optionalWrite', 'with', 'without', 'added' or 'changed')",
                                                  option));
    }
    std::vector<TermDeclaration> terms;
    for (const auto& [name, kind] : kTerms) {
        const auto found = std::ranges::find(*entries, name, [](const luau::Table::Entry& entry) {
            return std::string_view{entry.first.as<std::string>()};
        });
        if (found == entries->end()) {
            continue;
        }
        auto components = readNames(source.system, found->second, std::format("query.{}", name));
        if (!components) {
            return tl::unexpected{std::move(components.error())};
        }
        for (auto& component : *components) {
            terms.push_back(TermDeclaration{.kind = kind, .component = std::move(component)});
        }
    }
    return terms;
}

/// @brief What the options of a declaration give, before the required ones are checked.
struct Options {
    std::optional<ecs::Phase> phase;
    std::optional<std::vector<TermDeclaration>> terms;
    std::optional<luau::FunctionRef> run;
    std::vector<std::string> after;
    std::vector<std::string> before;
};

/// @brief Reads one option of the declaration table into `options`.
[[nodiscard]] Result<void> readOption(const std::string_view system, const std::string& option,
                                      const luau::Value& value, Options& options) {
    if (option == "phase") {
        const auto phase = readPhase(system, value);
        if (!phase) {
            return tl::unexpected{phase.error()};
        }
        options.phase = *phase;
    } else if (option == "query") {
        auto terms = readQuery({.system = system}, value);
        if (!terms) {
            return tl::unexpected{std::move(terms.error())};
        }
        options.terms = std::move(*terms);
    } else if (option == "after" || option == "before") {
        auto names = readNames(system, value, option);
        if (!names) {
            return tl::unexpected{std::move(names.error())};
        }
        (option == "after" ? options.after : options.before) = std::move(*names);
    } else if (option == "run") {
        const auto* function = value.tryAs<luau::FunctionRef>();
        if (function == nullptr) {
            return invalid(system, "'run' must be a function");
        }
        options.run = *function;
    } else {
        return invalid(system, std::format("unknown option '{}' (expected 'phase', 'query', 'after', 'before' or "
                                           "'run')",
                                           option));
    }
    return {};
}
}  // namespace

Result<SystemDeclaration> readSystemDeclaration(std::string name, const luau::Table& spec) {
    const auto entries = namedEntries(name, spec, "the declaration");
    if (!entries) {
        return tl::unexpected{entries.error()};
    }

    Options options;
    for (const auto& [key, value] : *entries) {
        if (auto read = readOption(name, key.as<std::string>(), value, options); !read) {
            return tl::unexpected{std::move(read.error())};
        }
    }
    if (!options.phase) {
        return invalid(name, "needs a 'phase'");
    }
    if (!options.terms) {
        return invalid(name, "needs a 'query' table");
    }
    if (!options.run) {
        return invalid(name, "needs a 'run' function");
    }
    return SystemDeclaration{
        .name = std::move(name),
        .phase = *options.phase,
        .terms = std::move(*options.terms),
        .after = std::move(options.after),
        .before = std::move(options.before),
        .run = std::move(*options.run),
    };
}
}  // namespace rtype::scripting::system
