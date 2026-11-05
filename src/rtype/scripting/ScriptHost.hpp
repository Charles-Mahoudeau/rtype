/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptHost
*/

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Export.hpp"
#include "component/ComponentLayout.hpp"
#include "plugin/PluginDeclaration.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/luau/Library.hpp"
#include "rtype/luau/Result.hpp"
#include "rtype/luau/Runtime.hpp"
#include "system/RuntimeHandle.hpp"

namespace rtype::scripting {
/// @brief Runs the scripts of a game on an App: owns their Luau runtime and the `engine` global they declare ECS data
/// through.
///
/// @details It knows no Plugin, so it can be used and tested with a bare App. `engine` holds the constant flags
/// `isClient`, `isServer` and `isDebug`, and the `ecs` library: `engine.ecs.component(name, {fields, defaults,
/// replication})` registers a component in the App's world (see readComponentDeclaration() for the table), and
/// `engine.ecs.system(name, {phase, query, after, before, run})` adds a system to its schedule (see
/// readSystemDeclaration()); its body calls `run` with a `ctx` (see runScriptSystem()), among which `ctx.commands` (see
/// registerCommandsType()).
///
/// Scripts can group their components and systems in Luau plugins: `engine.ecs.plugin(name, {build, plugins})` declares
/// one (see readPluginDeclaration()) and `engine.ecs.addPlugin(name)` adds it. Adding a plugin first adds the plugins
/// it lists, then calls `build(app)`, where `app` has `component`, `system` and `addPlugin`, the same as `engine.ecs`.
/// A plugin is built once however many times it is added; a cycle, an unknown name or a name declared twice is a Luau
/// error naming the plugins.
///
/// The host and the App may be destroyed in either order. The systems a script added stay in the App when the host
/// is destroyed first, but their script is gone with the runtime: they can no longer run it.
class RTYPE_SCRIPTING_API ScriptHost {
  public:
    /// @brief What the scripts can tell about the process they run in, as the constant flags of `engine`.
    struct Options {
        bool isClient{false};  ///< `engine.isClient`: this process runs the client side (also true when standalone).
        bool isServer{false};  ///< `engine.isServer`: this process runs the server side (also true when standalone).
        bool isDebug{false};   ///< `engine.isDebug`: this is a debug run.
    };

    /// @brief Creates the runtime and the `engine` global.
    /// @param app The App scripts declare their ECS data on; it must outlive the host's use of it.
    /// @param options The flags of `engine`.
    /// @return The host, or the error of the runtime that could not be created.
    [[nodiscard]] static luau::Result<std::unique_ptr<ScriptHost>> create(ecs::App& app, Options options);

    ~ScriptHost();
    ScriptHost(const ScriptHost& other) = delete;
    ScriptHost& operator=(const ScriptHost& other) = delete;
    ScriptHost(ScriptHost&& other) = delete;
    ScriptHost& operator=(ScriptHost&& other) = delete;

    /// @brief Compiles a script and runs it, once.
    /// @param name Name of the script, for the error messages.
    /// @param source Luau source code.
    /// @return Nothing, or the error of the script: it does not compile, or it raised one (a bad declaration is one).
    [[nodiscard]] luau::Result<> run(std::string name, const std::string& source) const;

    /// @param name Name of a component a script declared.
    /// @return Its layout, with the default value a field left out of `Commands::add` or `spawn` takes, or null if no
    /// script declared it. Valid while the host lives.
    [[nodiscard]] const component::ComponentLayout* findComponent(std::string_view name) const noexcept;

  private:
    /// @param app The App scripts declare their ECS data on.
    /// @param runtime The runtime of the scripts.
    ScriptHost(ecs::App& app, luau::Runtime runtime) noexcept;

    /// @brief Builds `engine` and its `ecs` library, and makes it a global.
    [[nodiscard]] luau::Result<> installEngine(Options options);

    /// @brief The body of `engine.ecs.component`.
    /// @throws luau::exceptions::RuntimeError When the declaration is wrong or the component cannot be registered.
    void declareComponent(std::string name, const luau::Table& spec);

    /// @brief The body of `engine.ecs.system`.
    /// @throws luau::exceptions::RuntimeError When the declaration is wrong or the system cannot be added.
    void declareSystem(std::string name, const luau::Table& spec);

    /// @brief The body of `engine.ecs.plugin`.
    /// @throws luau::exceptions::RuntimeError When the declaration is wrong or the name is already declared.
    void declarePlugin(const std::string& name, const luau::Table& spec);

    /// @brief The body of `engine.ecs.addPlugin` and `app:addPlugin`: adds the plugins a plugin lists, then builds it.
    /// @throws luau::exceptions::RuntimeError When no plugin has this name, plugins depend on each other in a cycle, or
    /// a `build` raised an error.
    void addPlugin(const std::string& name);

    /// @brief Registers the `App` usertype that `build` receives.
    [[nodiscard]] luau::Result<> registerAppType();

    /// @brief A Luau plugin and how far its addition went.
    struct PluginEntry {
        /// @brief How far adding the plugin went.
        enum class State : std::uint8_t {
            kDeclared,  ///< Not added yet.
            kBuilding,  ///< Its plugins are being added or its `build` is running.
            kAdded,     ///< Done: adding it again does nothing.
        };
        plugin::PluginDeclaration declaration;  ///< What the script declared.
        State state{State::kDeclared};          ///< Where the addition is.
    };

    ecs::App& _app;          ///< The App scripts declare their ECS data on.
    luau::Runtime _runtime;  ///< The runtime of the scripts; declared before what holds references into it.
    std::shared_ptr<system::RuntimeHandle> _handle;  ///< Points to the runtime for the systems; cleared on destruction.
    std::optional<luau::Library> _engine;            ///< The `engine` library, with the sub-library `ecs`.
    component::ComponentLayouts _components;         ///< Components scripts declared.
    std::map<std::string, PluginEntry> _plugins;     ///< Luau plugins scripts declared, by name.
    std::vector<std::string> _pluginStack;           ///< Plugins being added, outermost first, to name a cycle.
};
}  // namespace rtype::scripting
