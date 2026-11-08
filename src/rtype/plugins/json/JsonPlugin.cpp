/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** JsonPlugin
*/

#include "plugins/json/JsonPlugin.hpp"

#include <array>
#include <span>
#include <string_view>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"

namespace rtype::plugins::json {

std::string_view JsonPlugin::getName() const noexcept { return kName; }

std::span<const std::string_view> JsonPlugin::getDependencies() const noexcept { return {}; }

void JsonPlugin::build(ecs::App& app) { addJsonForms(app.getPluginRegistry()); }

void addJsonForms(ecs::PluginRegistry& registry) {
    static constexpr auto kForms = std::to_array<char>({
#include "JsonForms.luau.h"
    });
    // bin2c ends the bytes with a null terminator, which is not part of the source.
    registry.addScriptForms("addJsonForms", std::string_view{kForms.data(), kForms.size() - 1});
}

}  // namespace rtype::plugins::json
