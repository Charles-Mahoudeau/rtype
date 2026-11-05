/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginRegistry
*/

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
/// @brief The plugins an App can build by name: a factory per plugin name.
/// @details The App uses it for the dependencies of a plugin that were not added (see Plugin::getDependencies()):
/// a required plugin registered here is built with its factory, one that is not is an error. The engine fills it with
/// the plugins it knows, from linked code or loaded libraries; a factory takes no argument, so a plugin built from it
/// has its default settings.
/// @code
/// app.getPluginRegistry().add("rtype.Transform", [] { return std::make_unique<TransformPlugin>(); });
/// @endcode
class RTYPE_ECS_API PluginRegistry {
  public:
    /// @brief Builds a plugin: called once per create(), never returns null.
    using Factory = std::function<std::unique_ptr<Plugin>()>;

    PluginRegistry() = default;
    ~PluginRegistry() = default;
    PluginRegistry(const PluginRegistry& other) = delete;
    PluginRegistry& operator=(const PluginRegistry& other) = delete;
    PluginRegistry(PluginRegistry&& other) noexcept = default;
    PluginRegistry& operator=(PluginRegistry&& other) noexcept = default;

    /// @return The registered names, sorted.
    [[nodiscard]] std::vector<std::string> getNames() const;

    /// @brief Registers a factory under a plugin's name, replacing any previous one with the same name.
    /// @param name The name the plugin built by @p factory reports (Plugin::getName()).
    void add(std::string name, Factory factory);

    /// @return Whether a factory is registered under this name.
    [[nodiscard]] bool has(std::string_view name) const noexcept;

    /// @return A new plugin built by the factory registered under @p name, or null if none is.
    [[nodiscard]] std::unique_ptr<Plugin> create(std::string_view name) const;

  private:
    std::map<std::string, Factory, std::less<>> _factories;  ///< Factories by plugin name.
};
}  // namespace rtype::ecs
