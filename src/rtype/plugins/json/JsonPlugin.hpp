/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** JsonPlugin
*/

#pragma once

#include <span>
#include <string_view>

#include "plugins/json/Export.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"

namespace rtype::plugins::json {

/// @brief JSON for scripts: the `engine.json` helpers (`encode`, `decode`, `tryDecode`, `map`) of `JsonForms.luau`
/// (docs/Json.md). It has no component, resource, event or system, and needs no other plugin; it runs on every side.
class RTYPE_PLUGIN_JSON_API JsonPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.Json";  ///< The plugin's name.

    JsonPlugin() = default;
    ~JsonPlugin() override = default;
    JsonPlugin(const JsonPlugin& other) = default;
    JsonPlugin& operator=(const JsonPlugin& other) = default;
    JsonPlugin(JsonPlugin&& other) noexcept = default;
    JsonPlugin& operator=(JsonPlugin&& other) noexcept = default;

    /// @return "rtype.Json".
    [[nodiscard]] std::string_view getName() const noexcept override;
    /// @return No plugin: it needs none.
    [[nodiscard]] std::span<const std::string_view> getDependencies() const noexcept override;
    /// @brief Registers the `engine.json` helpers (addJsonForms()).
    void build(ecs::App& app) override;
};

/// @brief Registers the Luau helpers of `engine.json` on @p registry. Its library does it on every side, and so does
/// building the plugin, for a program without libraries.
RTYPE_PLUGIN_JSON_API void addJsonForms(ecs::PluginRegistry& registry);

}  // namespace rtype::plugins::json
