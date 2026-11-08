/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** JsonPlugin
*/

#include "plugins/json/JsonPlugin.hpp"

#include <span>
#include <string_view>

#include "plugins/json/JsonCalls.hpp"
#include "rtype/ecs/app/App.hpp"

namespace rtype::plugins::json {

std::string_view JsonPlugin::getName() const noexcept { return kName; }

std::span<const std::string_view> JsonPlugin::getDependencies() const noexcept { return {}; }

void JsonPlugin::build(ecs::App& app) { addJsonCalls(app); }

}  // namespace rtype::plugins::json
