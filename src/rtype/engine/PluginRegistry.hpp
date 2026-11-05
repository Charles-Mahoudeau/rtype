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

#include "Export.hpp"
#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::engine {
/// @brief The plugins a project can list by name in its settings (`plugins.client = { "backend", "audio" }`), each
/// built by a factory that knows what the plugin needs (a registry, a section of the settings).
///
/// @details Same shape as the backend and audio registries: the engine registers its plugins once, then creates the
/// ones the project names. An unknown name is an error naming it, so a typo in `project.luau` is reported at startup.
/// @code
/// rtype::engine::PluginRegistry registry;
/// registry.add("particle", [] { return std::make_unique<rtype::plugins::particle::ParticlePlugin>(); });
/// app.addPlugin(registry.create("particle"));
/// @endcode
class RTYPE_ENGINE_API PluginRegistry {
  public:
    /// @brief Builds a plugin: called once per create().
    using Factory = std::function<std::unique_ptr<ecs::Plugin>()>;

    PluginRegistry() = default;
    ~PluginRegistry() = default;
    PluginRegistry(const PluginRegistry& other) = delete;
    PluginRegistry& operator=(const PluginRegistry& other) = delete;
    PluginRegistry(PluginRegistry&& other) noexcept = default;
    PluginRegistry& operator=(PluginRegistry&& other) noexcept = default;

    /// @return The registered names, sorted.
    [[nodiscard]] std::vector<std::string> getNames() const;

    /// @brief Registers a plugin under a name, replacing any previous one with the same name.
    void add(std::string name, Factory factory);

    /// @return Whether a plugin is registered under this name.
    [[nodiscard]] bool has(std::string_view name) const;

    /// @return A new plugin built by the factory registered under @p name.
    /// @throws exceptions::RuntimeError If no plugin has this name; the message names it and lists the known ones.
    [[nodiscard]] std::unique_ptr<ecs::Plugin> create(std::string_view name) const;

  private:
    std::map<std::string, Factory, std::less<>> _factories;  ///< Factories by name.
};
}  // namespace rtype::engine
