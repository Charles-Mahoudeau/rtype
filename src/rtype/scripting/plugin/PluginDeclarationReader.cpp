/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginDeclarationReader
*/

#include "PluginDeclarationReader.hpp"

#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>
#include <vector>

#include "PluginDeclaration.hpp"
#include "rtype/luau/FunctionRef.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/scripting/Error.hpp"
#include "rtype/scripting/ErrorKind.hpp"
#include "rtype/scripting/Result.hpp"

namespace rtype::scripting::plugin {
namespace {
[[nodiscard]] tl::unexpected<Error> invalid(const std::string_view plugin, const std::string& what) {
    return fail(ErrorKind::kInvalidDeclaration, std::format("Plugin '{}': {}", plugin, what));
}

/// @brief The strings of the `plugins` list, in order.
[[nodiscard]] Result<std::vector<std::string>> readPlugins(const std::string_view plugin, const luau::Value& value) {
    const auto* table = value.tryAs<luau::Table>();
    if (table == nullptr) {
        return invalid(plugin, "'plugins' must be a list of plugin names");
    }
    const auto size = table->size();
    const auto count = table->count();
    if (!size || !count) {
        return invalid(plugin, "cannot read 'plugins'");
    }
    if (*size != *count) {
        return invalid(plugin, "'plugins' must be a list of plugin names, without holes or named keys");
    }
    std::vector<std::string> names;
    for (std::size_t index = 1; index <= *size; ++index) {
        const auto item = table->getAt(index);
        const auto* name = item ? item->tryAs<std::string>() : nullptr;
        if (name == nullptr) {
            return invalid(plugin, "'plugins' must be a list of plugin names");
        }
        names.push_back(*name);
    }
    return names;
}
}  // namespace

Result<PluginDeclaration> readPluginDeclaration(const std::string& name, const luau::Table& spec) {
    if (name.empty()) {
        return fail(ErrorKind::kInvalidDeclaration, "Plugin: the name must not be empty");
    }
    const auto entries = spec.entries();
    if (!entries) {
        return invalid(name, std::format("cannot read the declaration: {}", entries.error().message()));
    }
    PluginDeclaration declaration{.name = name};
    for (const auto& [key, value] : *entries) {
        const auto* option = key.tryAs<std::string>();
        if (option == nullptr) {
            return invalid(name, "the keys of the declaration must be strings");
        }
        if (*option == "build") {
            const auto* function = value.tryAs<luau::FunctionRef>();
            if (function == nullptr) {
                return invalid(name, "'build' must be a function");
            }
            declaration.build = *function;
        } else if (*option == "plugins") {
            auto plugins = readPlugins(name, value);
            if (!plugins) {
                return tl::unexpected{std::move(plugins.error())};
            }
            declaration.plugins = std::move(*plugins);
        } else {
            return invalid(name, std::format("unknown option '{}' (expected 'build' or 'plugins')", *option));
        }
    }
    return declaration;
}
}  // namespace rtype::scripting::plugin
