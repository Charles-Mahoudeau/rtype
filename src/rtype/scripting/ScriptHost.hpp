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
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "Export.hpp"
#include "component/ComponentLayout.hpp"
#include "plugin/PluginDeclaration.hpp"
#include "rtype/core/reload/ReloadOutcome.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/luau/CallContext.hpp"
#include "rtype/luau/Library.hpp"
#include "rtype/luau/ModuleLoader.hpp"
#include "rtype/luau/ModuleSource.hpp"
#include "rtype/luau/Result.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/ValuePack.hpp"
#include "system/RuntimeHandle.hpp"
#include "system/SystemRegistration.hpp"

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
/// `engine.call(name, fields)` calls an engine call a plugin registered on the App (ecs::Calls) right away, with the
/// fields of its arguments (see call::callEngine()); an unknown name is a Luau error. A system makes calls at each run
/// through `ctx.call` instead, for the calls it lists under `calls`. `engine.hasCall(name)` tells whether a call exists
/// and `engine.assetId(path)` gives the id of an asset. `engine.assets` and `engine.input` hold the setup forms, which
/// make those calls from a declarative table: each plugin brings its own (ecs::PluginRegistry::addScriptForms()),
/// written with `engine.forms` (setup::getFormsPrelude()), and they run before the next script.
///
/// Hot reload: reloadScript() runs a script again while the game runs and swaps what it declares in the App, keeping
/// the world: its systems replace the systems of the same name (in the same phase, with their new order), a system it
/// no longer declares is removed, a component whose layout did not change takes its new defaults, and a Luau plugin it
/// declares again is built again. What cannot be swapped in place (a component with a new layout, a component that
/// disappeared, a new replicated component, a system that changed phase) is reported as a rebuild, and a script that
/// raises an error changes nothing. The state a system kept in its script (an upvalue) starts again from the new code.
///
/// Modules: a host created with a resolver (the ScriptResolver of a game) gives its scripts `require` (see
/// require()). Each file is a module of its own, named by its chunk name (`scripts/<key>`), and owns what it declares,
/// so a nested `require` restores the requiring file as owner afterwards. For now the modules share their global
/// writes (the loader's `shareGlobals`), so a game that passes data through globals still works. A host without a
/// resolver has no `require`: run() and reloadScript() then run the given source directly, as one script of that name
/// (what the tests of the host do).
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
    /// @param resolver Maps the `require` paths of the scripts to their source (a modules::ScriptResolver). Empty: no
    /// `require`; see the class.
    /// @return The host, or the error of the runtime that could not be created.
    [[nodiscard]] static luau::Result<std::unique_ptr<ScriptHost>> create(ecs::App& app, Options options,
                                                                          luau::ModuleLoader::Resolver resolver = {});

    ~ScriptHost();
    ScriptHost(const ScriptHost& other) = delete;
    ScriptHost& operator=(const ScriptHost& other) = delete;
    ScriptHost(ScriptHost&& other) = delete;
    ScriptHost& operator=(ScriptHost&& other) = delete;

    /// @brief Requires a module from outside any script (a root require): runs it if it did not run yet, after the
    /// setup forms.
    ///
    /// @details What the module declares belongs to it, by its chunk name. A module already loaded (because another
    /// one required it) is not run again.
    /// @param path Path given to the resolver, with no requiring file: `./<key without extension>` for a file.
    /// @return Nothing, or the error (`require("<path>"): <reason>`); also if the host has no resolver.
    [[nodiscard]] luau::Result<> require(std::string_view path);

    /// @brief Compiles a script and runs it, once, directly: it has no `require`, and the host's modules do not know
    /// it.
    /// @param name Name of the script, for the error messages.
    /// @param source Luau source code.
    /// @return Nothing, or the error of the script: it does not compile, or it raised one (a bad declaration is one).
    [[nodiscard]] luau::Result<> run(std::string name, const std::string& source);

    /// @brief Runs a script again and swaps what it declares in the App (see the class).
    ///
    /// @details A script the host did not run before is added: its systems join the schedule. The App's schedules are
    /// only touched when the whole script ran. With a resolver and a name of the form `scripts/<key>`, the script is a
    /// module: its cache entry is replaced in a loader transaction (committed when applied, rolled back otherwise), so
    /// later requires see the new value. The modules that required it are not run again.
    /// @param name Name of the script, as given to run(), or its chunk name (`scripts/<key>`).
    /// @param source The new Luau source code.
    /// @return `applied` with what changed ("system game.move"), `failed` with the error of the script (nothing was
    /// changed), or `needsRebuild` with why it cannot be applied in place.
    [[nodiscard]] core::reload::ReloadOutcome reloadScript(const std::string& name, const std::string& source);

    /// @brief Forgets a script that was deleted: removes its systems from the App.
    /// @param name Name of the script, as given to run(), or its chunk name.
    /// @return As reloadScript(): a component or a plugin of the script cannot be removed in place.
    [[nodiscard]] core::reload::ReloadOutcome removeScript(const std::string& name);

    /// @param name Name of a component a script declared.
    /// @return Its layout, with the default value a field left out of `Commands::add` or `spawn` takes, or null if no
    /// script declared it. Valid while the host lives.
    [[nodiscard]] const component::ComponentLayout* findComponent(std::string_view name) const;

  private:
    /// @brief Compiles a script and runs it, once, without loading the forms first.
    [[nodiscard]] luau::Result<> runSource(std::string name, const std::string& source) const;

    /// @brief Runs the setup forms of the App's plugins that did not run yet.
    [[nodiscard]] luau::Result<> loadForms() const;

    /// @param app The App scripts declare their ECS data on.
    /// @param runtime The runtime of the scripts.
    ScriptHost(ecs::App& app, luau::Runtime runtime, luau::ModuleLoader::Resolver resolver);

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

    /// @brief The body of `engine.call`: calls an engine call by name, outside any system.
    /// @throws luau::exceptions::RuntimeError When no call has the name, or the call fails (see call::callEngine()).
    [[nodiscard]] luau::ValuePack callEngine(const luau::CallContext& arguments);

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
        std::string file;                       ///< The script that declared it.
    };

    /// @brief What one script declared, to tell what a new version of it removed.
    struct FileDeclarations {
        std::set<std::string> components;           ///< The components it declared.
        std::map<std::string, ecs::Phase> systems;  ///< The systems it declared, with their phase.
    };

    /// @brief What a reload in progress holds back until the whole script ran.
    struct Staging {
        std::map<std::string, FileDeclarations> declared;    ///< What was declared, by the script that owns it.
        std::vector<system::BuiltSystem> systems;            ///< The systems made, not in the schedule yet.
        std::map<std::string, PluginEntry> replacedPlugins;  ///< The plugins declared again, as they were before.
        std::set<std::string> declaredPlugins;               ///< The plugins the reload declared.
        std::optional<std::string> rebuild;                  ///< Why the reload cannot be applied in place.
    };

    /// @brief The reload of one script, from running it to applying it: @p source is null for a script that was
    /// deleted.
    [[nodiscard]] core::reload::ReloadOutcome reloadFile(const std::string& file, const std::string* source);

    /// @brief reloadFile() without the loader transaction around it.
    [[nodiscard]] core::reload::ReloadOutcome applyReload(const std::string& file, const std::string* source);

    /// @return The module key of a script name (`scripts/<key>`), or nothing if the script is not a module of this
    /// host.
    [[nodiscard]] std::optional<std::string> moduleKeyOf(std::string_view name) const;

    /// @brief Resolves through the host's resolver, giving the source being reloaded in place of the file's.
    [[nodiscard]] luau::Result<luau::ModuleSource> resolveModule(std::string_view requester, std::string_view path);

    /// @brief Makes the script of a module the owner of what is declared, or gives the previous owner back.
    void enterModule(std::string_view key, bool entering);

    /// @brief Puts the plugins back as they were before the reload in progress.
    void restorePlugins();

    /// @return The phase of the system this name, in the scripts' records; nothing if no script declared it.
    [[nodiscard]] std::optional<ecs::Phase> findSystemPhase(const std::string& name) const;

    ecs::App& _app;          ///< The App scripts declare their ECS data on.
    luau::Runtime _runtime;  ///< The runtime of the scripts; declared before what holds references into it.
    std::shared_ptr<system::RuntimeHandle> _handle;  ///< Points to the runtime for the systems; cleared on destruction.
    std::optional<luau::Library> _engine;            ///< The `engine` library, with the sub-library `ecs`.
    mutable std::set<std::string> _loadedForms;      ///< The setup forms already run, by name.
    component::ComponentLayouts _components;         ///< Components scripts declared.
    std::map<std::string, PluginEntry> _plugins;     ///< Luau plugins scripts declared, by name.
    std::vector<std::string> _pluginStack;           ///< Plugins being added, outermost first, to name a cycle.
    std::map<std::string, FileDeclarations> _files;  ///< What each script declared, by its name.
    std::string _owner;                              ///< The script a declaration made now belongs to.
    std::unique_ptr<Staging> _staging;               ///< The reload in progress; null outside a reload.
    luau::ModuleLoader::Resolver _resolver;          ///< The game's resolver; empty without modules.
    std::map<std::string, std::string> _chunkNames;  ///< Chunk name of each module key the resolver gave.
    std::map<std::string, std::string> _overrides;   ///< Source a reload gives in place of a module's file, by key.
    std::vector<std::string> _ownerStack;            ///< The owners to give back when a module ends, innermost last.
    std::unique_ptr<luau::ModuleLoader> _loader;     ///< The module cache; null without a resolver. After `_runtime`.
};
}  // namespace rtype::scripting
