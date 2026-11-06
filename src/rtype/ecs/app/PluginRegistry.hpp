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

#include "core/config/Settings.hpp"
#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
/// @brief The plugins an App can build by name: a factory per plugin name.
/// @details The App uses it for the dependencies of a plugin that were not added (see Plugin::getDependencies()):
/// a required plugin registered here is built with its factory, one that is not is an error. The engine fills it with
/// the plugins it knows, from linked code or loaded libraries, and gives it the project's settings: a factory reads its
/// own section from them, and one that takes no argument builds a plugin with its default settings.
/// @code
/// app.getPluginRegistry().add("rtype.Transform", [] { return std::make_unique<TransformPlugin>(); });
/// app.getPluginRegistry().add("rtype.Shader", [](const rtype::core::config::Settings& settings) {
///     return std::make_unique<ShaderPlugin>(settings);
/// });
/// @endcode
class RTYPE_ECS_API PluginRegistry {
  public:
    /// @brief Builds a plugin from the project's settings: called once per create(), never returns null.
    using Factory = std::function<std::unique_ptr<Plugin>(const core::config::Settings&)>;
    /// @brief Builds a plugin that reads no settings: called once per create(), never returns null.
    using DefaultFactory = std::function<std::unique_ptr<Plugin>()>;

    PluginRegistry() = default;
    ~PluginRegistry() = default;
    PluginRegistry(const PluginRegistry& other) = delete;
    PluginRegistry& operator=(const PluginRegistry& other) = delete;
    PluginRegistry(PluginRegistry&& other) noexcept = default;
    PluginRegistry& operator=(PluginRegistry&& other) noexcept = default;

    /// @brief Sets what the factories read.
    /// @param settings The project's settings, whole.
    void setSettings(core::config::Settings settings);

    /// @return The registered names, sorted.
    [[nodiscard]] std::vector<std::string> getNames() const;

    /// @brief Registers a factory under a plugin's name, replacing any previous one with the same name.
    /// @param name The name the plugin built by @p factory reports (Plugin::getName()).
    void add(std::string name, Factory factory);

    /// @brief Registers a factory that reads no settings, as add().
    void add(std::string name, DefaultFactory factory);

    /// @return Whether a factory is registered under this name.
    [[nodiscard]] bool has(std::string_view name) const noexcept;

    /// @brief Registers Luau source a script host runs before the game's scripts: the setup forms of a plugin, such as
    /// `engine.assets.texture`. A plugin library registers them even if its plugin is not added, so a server that has
    /// no window still gets the id of a texture.
    /// @param name What the source is known by; a name registered twice keeps the first source.
    /// @param source The Luau source.
    void addScriptForms(std::string name, std::string_view source);

    /// @return The sources of addScriptForms(), by name.
    [[nodiscard]] const std::map<std::string, std::string, std::less<>>& getScriptForms() const noexcept;

    /// @return A new plugin built by the factory registered under @p name, or null if none is.
    [[nodiscard]] std::unique_ptr<Plugin> create(std::string_view name) const;

  private:
    core::config::Settings _settings;                        ///< What the factories read.
    std::map<std::string, Factory, std::less<>> _factories;  ///< Factories by plugin name.
    std::map<std::string, std::string, std::less<>> _forms;  ///< Setup forms of the plugins, by name.
};
}  // namespace rtype::ecs
