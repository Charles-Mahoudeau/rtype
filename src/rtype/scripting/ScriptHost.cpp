/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptHost
*/

#include "ScriptHost.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>
#include <vector>

#include "call/EngineCall.hpp"
#include "commands/ScriptCommands.hpp"
#include "component/ComponentLayout.hpp"
#include "component/ComponentRegistration.hpp"
#include "component/DeclarationReader.hpp"
#include "core/asset/AssetId.hpp"
#include "plugin/PluginDeclarationReader.hpp"
#include "rtype/core/reload/ReloadOutcome.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/call/CallInfo.hpp"
#include "rtype/ecs/call/Calls.hpp"
#include "rtype/ecs/exceptions/ScheduleException.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/Schedule.hpp"
#include "rtype/ecs/schedule/Schedules.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/world/World.hpp"
#include "rtype/luau/CallContext.hpp"
#include "rtype/luau/ErrorKind.hpp"
#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/Failure.hpp"
#include "rtype/luau/Function.hpp"
#include "rtype/luau/LibraryBuilder.hpp"
#include "rtype/luau/ModuleSource.hpp"
#include "rtype/luau/Ref.hpp"
#include "rtype/luau/Result.hpp"
#include "rtype/luau/RuntimeConfig.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Usertype.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"
#include "rtype/scripting/ErrorKind.hpp"
#include "setup/FormsPrelude.hpp"
#include "system/ContextTypes.hpp"
#include "system/RuntimeHandle.hpp"
#include "system/SystemDeclarationReader.hpp"
#include "system/SystemRegistration.hpp"
#include "view/ViewTypes.hpp"

namespace rtype::scripting {
namespace {
/// @return The configuration of the scripts' runtime: the standard libraries (the runtime opens all of them or none).
luau::RuntimeConfig makeRuntimeConfig() {
    return {
        .libs = luau::RuntimeConfig::Libs::kStandard,
        .optimizationLevel = luau::RuntimeConfig::OptimizationLevel::kStandard,
    };
}

/// @brief What `app` holds in a plugin's `build`: the host to declare on, and whether the build is still running.
struct AppHandle {
    ScriptHost* host{nullptr};    ///< The host the declarations go to.
    std::shared_ptr<bool> alive;  ///< False once `build` returned, so a script cannot keep `app` for later.
};

/// @brief Raises a Luau error unless the build that received the handle is still running.
void checkAlive(const AppHandle& handle) {
    if (!*handle.alive) {
        throw luau::exceptions::RuntimeError{"the app is no longer valid: it only lives during the plugin's build"};
    }
}
}  // namespace

luau::Result<std::unique_ptr<ScriptHost>> ScriptHost::create(ecs::App& app, const Options options,
                                                             luau::ModuleLoader::Resolver resolver) {
    auto runtime = luau::Runtime::create(makeRuntimeConfig());
    if (!runtime) {
        return tl::unexpected{std::move(runtime.error())};
    }
    // The constructor is private, so make_unique cannot reach it.
    std::unique_ptr<ScriptHost> host{new ScriptHost{app, std::move(*runtime), std::move(resolver)}};
    if (auto installed = host->installEngine(options); !installed) {
        return tl::unexpected{std::move(installed.error())};
    }
    return host;
}

ScriptHost::ScriptHost(ecs::App& app, luau::Runtime runtime, luau::ModuleLoader::Resolver resolver)
    : _app{app},
      _runtime{std::move(runtime)},
      _handle{std::make_shared<system::RuntimeHandle>()},
      _resolver{std::move(resolver)} {
    _handle->runtime = &_runtime;
    _handle->components = &_components;
    if (_resolver) {
        // The modules share their global writes until the sandbox gives each file its own globals.
        _loader = std::make_unique<luau::ModuleLoader>(
            _runtime,
            [this](const std::string_view requester, const std::string_view path) {
                return resolveModule(requester, path);
            },
            [this](const std::string_view key, const bool entering) { enterModule(key, entering); },
            luau::ModuleLoader::Options{.shareGlobals = true});
    }
}

ScriptHost::~ScriptHost() {
    _loader.reset();
    _handle->runtime = nullptr;
}

luau::Result<luau::ModuleSource> ScriptHost::resolveModule(const std::string_view requester,
                                                           const std::string_view path) {
    auto module = _resolver(requester, path);
    if (!module) {
        return module;
    }
    _chunkNames.insert_or_assign(module->key, module->chunkName);
    if (const auto replaced = _overrides.find(module->key); replaced != _overrides.end()) {
        module->source = replaced->second;
    }
    return module;
}

void ScriptHost::enterModule(const std::string_view key, const bool entering) {
    if (entering) {
        _ownerStack.push_back(std::exchange(_owner, _chunkNames[std::string{key}]));
    } else if (!_ownerStack.empty()) {
        _owner = std::move(_ownerStack.back());
        _ownerStack.pop_back();
    }
}

luau::Result<> ScriptHost::require(const std::string_view path) {
    if (!_loader) {
        return luau::Failure{luau::ErrorKind::kRuntime, "this host has no modules: it was created without a resolver"};
    }
    if (auto loaded = loadForms(); !loaded) {
        return loaded;
    }
    auto module = _loader->require(path);
    if (!module) {
        return tl::unexpected{std::move(module.error())};
    }
    return {};
}

std::optional<std::string> ScriptHost::moduleKeyOf(const std::string_view name) const {
    constexpr std::string_view kPrefix = "scripts/";
    if (!_loader || !name.starts_with(kPrefix) || name.size() == kPrefix.size()) {
        return std::nullopt;
    }
    return std::string{name.substr(kPrefix.size())};
}

luau::Result<> ScriptHost::run(std::string name, const std::string& source) {
    if (auto loaded = loadForms(); !loaded) {
        return loaded;
    }
    _owner = name;
    auto result = runSource(std::move(name), source);
    _owner.clear();
    return result;
}

luau::Result<> ScriptHost::runSource(std::string name, const std::string& source) const {
    auto script = _runtime.load(std::move(name), source);
    if (!script) {
        return tl::unexpected{std::move(script.error())};
    }
    return _runtime.run(*script);
}

luau::Result<> ScriptHost::loadForms() const {
    for (const auto& [name, source] : _app.getPluginRegistry().getScriptForms()) {
        if (_loadedForms.insert(name).second) {
            if (auto ran = runSource(name, source); !ran) {
                return ran;
            }
        }
    }
    return {};
}

const component::ComponentLayout* ScriptHost::findComponent(const std::string_view name) const {
    // The map is keyed by std::string and has no transparent lookup.
    const auto found = _components.find(std::string{name});
    return found == _components.end() ? nullptr : &found->second;
}

luau::Result<> ScriptHost::installEngine(const Options options) {
    if (auto registered = view::registerViewTypes(_runtime); !registered) {
        return registered;
    }
    if (auto registered = commands::registerCommandsType(_runtime); !registered) {
        return registered;
    }
    if (auto registered = system::registerContextTypes(_runtime); !registered) {
        return registered;
    }
    if (auto registered = registerAppType(); !registered) {
        return registered;
    }
    luau::LibraryBuilder ecsBuilder{_runtime};
    ecsBuilder.setName("ecs");
    ecsBuilder.function("component").bind([this](std::string name, const luau::Table& spec) {
        declareComponent(std::move(name), spec);
        return luau::ValuePack{};
    });
    ecsBuilder.function("system").bind([this](std::string name, const luau::Table& spec) {
        declareSystem(std::move(name), spec);
        return luau::ValuePack{};
    });
    ecsBuilder.function("plugin").bind([this](const std::string& name, const luau::Table& spec) {
        declarePlugin(name, spec);
        return luau::ValuePack{};
    });
    ecsBuilder.function("addPlugin").bind([this](const std::string& name) {
        addPlugin(name);
        return luau::ValuePack{};
    });

    luau::LibraryBuilder engineBuilder{_runtime};
    engineBuilder.setName("engine");
    engineBuilder.adoptLibrary(ecsBuilder.build());
    engineBuilder.function("hasCall").bind([this](const std::string& name) {
        luau::ValuePack result;
        result.add(luau::Value{_app.getWorld().getCalls().findByName(name) != nullptr});
        return result;
    });
    engineBuilder.function("assetId").setHandler([this](const luau::CallContext& arguments) {
        const auto* path = arguments.args().empty() ? nullptr : arguments.args().front().tryAs<std::string>();
        if (path == nullptr) {
            throw luau::exceptions::RuntimeError{"engine.assetId expects a path"};
        }
        auto id = _runtime.usertype<core::asset::AssetId>().create(core::asset::AssetId::fromPath(*path));
        if (!id) {
            throw luau::exceptions::RuntimeError{std::string{id.error().message()}};
        }
        luau::ValuePack result;
        result.add(luau::Value{std::move(*id)});
        return result;
    });
    engineBuilder.function("call").setHandler(
        [this](const luau::CallContext& arguments) { return callEngine(arguments); });
    _engine.emplace(engineBuilder.build());

    luau::Table engine = _engine->table();
    for (const auto& [name, flag] : {
             std::pair<std::string_view, bool>{"isClient", options.isClient},
             {"isServer", options.isServer},
             {"isDebug", options.isDebug},
         }) {
        if (auto set = engine.set(name, luau::Value{flag}); !set) {
            return set;
        }
    }
    if (auto registered = _runtime.globals().registerLibrary(*_engine); !registered) {
        return registered;
    }
    return runSource(std::string{setup::kFormsPreludeName}, std::string{setup::getFormsPrelude()});
}

void ScriptHost::declareComponent(std::string name, const luau::Table& spec) {
    const auto declaration = component::readComponentDeclaration(std::move(name), spec);
    if (!declaration) {
        throw luau::exceptions::RuntimeError{std::string{declaration.error().message()}};
    }
    // A reload can only add what the world already agrees with: a replicated component the network plugin did not see
    // when it was built is not one of them.
    if (_staging && !_components.contains(declaration->name) &&
        declaration->replication != ecs::ReplicationMode::kNone) {
        _staging->rebuild = std::format("component '{}' is new and replicated", declaration->name);
    }
    auto layout = component::registerComponent(_app.getWorld().getComponents(), *declaration);
    if (!layout) {
        if (_staging && (layout.error().kind() == ErrorKind::kLayoutMismatch ||
                         layout.error().kind() == ErrorKind::kRegistration)) {
            _staging->rebuild = std::format("component '{}' changed: {}", declaration->name, layout.error().message());
        }
        throw luau::exceptions::RuntimeError{std::string{layout.error().message()}};
    }
    (_staging ? _staging->declared[_owner] : _files[_owner]).components.insert(declaration->name);
    _components.insert_or_assign(declaration->name, std::move(*layout));
}

void ScriptHost::declareSystem(std::string name, const luau::Table& spec) {
    auto declaration = system::readSystemDeclaration(std::move(name), spec);
    if (!declaration) {
        throw luau::exceptions::RuntimeError{std::string{declaration.error().message()}};
    }
    const std::string systemName = declaration->name;
    const ecs::Phase phase = declaration->phase;
    if (!_staging) {
        if (auto registered = system::registerSystem(_app, std::move(*declaration), _handle); !registered) {
            throw luau::exceptions::RuntimeError{std::string{registered.error().message()}};
        }
        _files[_owner].systems.insert_or_assign(systemName, phase);
        return;
    }
    for (const auto& [file, declared] : _files) {
        if (file != _owner && declared.systems.contains(systemName)) {
            throw luau::exceptions::RuntimeError{
                std::format("System '{}': already declared by the script '{}'", systemName, file)};
        }
    }
    if (std::ranges::any_of(_staging->systems,
                            [&systemName](const auto& built) { return built.spec.system.getName() == systemName; })) {
        throw luau::exceptions::RuntimeError{std::format("System '{}': declared twice", systemName)};
    }
    auto built = system::makeSystem(_app, std::move(*declaration), _handle);
    if (!built) {
        throw luau::exceptions::RuntimeError{std::string{built.error().message()}};
    }
    if (const std::optional<ecs::Side> side = built->side) {
        built->spec.system.setSide(*side);
    }
    _staging->declared[_owner].systems.insert_or_assign(systemName, phase);
    _staging->systems.push_back(std::move(*built));
}

void ScriptHost::declarePlugin(const std::string& name, const luau::Table& spec) {
    auto declaration = plugin::readPluginDeclaration(name, spec);
    if (!declaration) {
        throw luau::exceptions::RuntimeError{std::string{declaration.error().message()}};
    }
    const std::string key = declaration->name;
    const auto known = _plugins.find(key);
    if (known != _plugins.end()) {
        // A reload may declare a plugin of its own script again; the old one is kept to put it back on failure.
        if (!_staging || known->second.file != _owner || _staging->declaredPlugins.contains(key)) {
            throw luau::exceptions::RuntimeError{std::format("Plugin '{}': already declared", key)};
        }
        _staging->replacedPlugins.emplace(key, std::move(known->second));
        _plugins.erase(known);
    }
    if (_staging) {
        _staging->declaredPlugins.insert(key);
    }
    _plugins.emplace(
        key,
        PluginEntry{.declaration = std::move(*declaration), .state = PluginEntry::State::kDeclared, .file = _owner});
}

// NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the plugin dependencies, and cycles are detected.
void ScriptHost::addPlugin(const std::string& name) {
    const auto found = _plugins.find(name);
    if (found == _plugins.end()) {
        throw luau::exceptions::RuntimeError{std::format("Plugin '{}': no script declared it", name)};
    }
    PluginEntry& entry = found->second;
    if (entry.state == PluginEntry::State::kAdded) {
        return;
    }
    if (entry.state == PluginEntry::State::kBuilding) {
        std::string cycle;
        for (auto at = std::ranges::find(_pluginStack, name); at != _pluginStack.end(); ++at) {
            cycle += std::format("{} -> ", *at);
        }
        throw luau::exceptions::RuntimeError{std::format("Plugin cycle: {}{}", cycle, name)};
    }

    entry.state = PluginEntry::State::kBuilding;
    _pluginStack.push_back(name);
    const auto abandon = [&] {
        entry.state = PluginEntry::State::kDeclared;
        _pluginStack.pop_back();
    };
    try {
        // Map nodes never move, so `entry` stays valid while nested plugins are declared and added.
        for (const std::string& dependency : entry.declaration.plugins) {
            addPlugin(dependency);
        }
        if (entry.declaration.build) {
            // What the build declares belongs to the script of the plugin, whoever adds it.
            const std::string addedBy = std::exchange(_owner, entry.file);
            const auto restoreOwner = [this, &addedBy](void*) { _owner = addedBy; };
            const std::unique_ptr<void, decltype(restoreOwner)> ownerScope{this, restoreOwner};
            const auto handle = std::make_shared<bool>(true);
            auto app = _runtime.usertype<AppHandle>().create(AppHandle{.host = this, .alive = handle});
            if (!app) {
                throw luau::exceptions::RuntimeError{std::string{app.error().message()}};
            }
            luau::ValuePack args;
            args.add(luau::Value{std::move(*app)});
            const auto result = luau::Function::fromRef(*entry.declaration.build, name).call(args);
            *handle = false;
            if (!result) {
                throw luau::exceptions::RuntimeError{
                    std::format("Plugin '{}': build failed: {}", name, result.error().message())};
            }
        }
    } catch (...) {
        abandon();
        throw;
    }
    entry.state = PluginEntry::State::kAdded;
    _pluginStack.pop_back();
}

luau::ValuePack ScriptHost::callEngine(const luau::CallContext& arguments) {
    const auto* name = arguments.argCount() > 0 ? arguments.args().front().tryAs<std::string>() : nullptr;
    if (name == nullptr) {
        throw luau::exceptions::RuntimeError{"engine.call expects the name of an engine call"};
    }
    ecs::World& world = _app.getWorld();
    const ecs::CallInfo* info = world.getCalls().findByName(*name);
    if (info == nullptr) {
        throw luau::exceptions::RuntimeError{std::format("unknown engine call '{}'", *name)};
    }
    const luau::Value* fields = arguments.argCount() > 1 ? &*std::next(arguments.args().begin()) : nullptr;
    return call::callEngine(_runtime, *info, fields,
                            [&world, info](const std::span<const std::byte> bytes, const std::span<std::byte> result) {
                                world.getCalls().invoke(world, info->id, bytes, result);
                            });
}

core::reload::ReloadOutcome ScriptHost::reloadScript(const std::string& name, const std::string& source) {
    if (auto loaded = loadForms(); !loaded) {
        return core::reload::ReloadOutcome::failed(std::string{loaded.error().message()});
    }
    return reloadFile(name, &source);
}

core::reload::ReloadOutcome ScriptHost::removeScript(const std::string& name) { return reloadFile(name, nullptr); }

std::optional<ecs::Phase> ScriptHost::findSystemPhase(const std::string& name) const {
    for (const auto& declared : _files | std::views::values) {
        if (const auto found = declared.systems.find(name); found != declared.systems.end()) {
            return found->second;
        }
    }
    return std::nullopt;
}

void ScriptHost::restorePlugins() {
    for (const std::string& name : _staging->declaredPlugins) {
        _plugins.erase(name);
    }
    for (auto& [name, entry] : _staging->replacedPlugins) {
        _plugins.insert_or_assign(name, std::move(entry));
    }
}

core::reload::ReloadOutcome ScriptHost::reloadFile(const std::string& file, const std::string* source) {
    const std::optional<std::string> key = moduleKeyOf(file);
    if (!key) {
        return applyReload(file, source);
    }
    // The module's cache entry is replaced together with what it declares: both or neither.
    _loader->begin();
    _loader->invalidate({*key});
    if (source != nullptr) {
        _overrides.insert_or_assign(*key, *source);
    }
    core::reload::ReloadOutcome outcome = applyReload(file, source);
    _overrides.erase(*key);
    if (outcome.status == core::reload::ReloadOutcome::Status::kApplied) {
        _loader->commit();
    } else {
        _loader->rollback();
    }
    return outcome;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity): one sequence of steps that must stay in this order.
core::reload::ReloadOutcome ScriptHost::applyReload(const std::string& file, const std::string* source) {
    using core::reload::ReloadOutcome;

    _staging = std::make_unique<Staging>();
    _owner = file;
    const auto finish = [this](void*) {
        _staging.reset();
        _owner.clear();
    };
    const std::unique_ptr<void, decltype(finish)> scope{this, finish};

    if (source != nullptr) {
        const std::optional<std::string> key = moduleKeyOf(file);
        std::string path;
        if (key) {
            path = "./" + key->substr(0, key->size() - std::string_view{".luau"}.size());
        }
        auto ran = key ? _loader->require(path).transform([](const luau::Ref&) {}) : runSource(file, *source);
        if (!ran) {
            if (_staging->rebuild) {
                return ReloadOutcome::needsRebuild(*_staging->rebuild);
            }
            restorePlugins();
            return ReloadOutcome::failed(std::string{ran.error().message()});
        }
        // A plugin declared again was built before: build it again, as the script's own addPlugin would have.
        for (const auto& [name, old] : _staging->replacedPlugins) {
            if (old.state != PluginEntry::State::kAdded) {
                continue;
            }
            try {
                addPlugin(name);
            } catch (const luau::exceptions::RuntimeError& error) {
                if (_staging->rebuild) {
                    return ReloadOutcome::needsRebuild(*_staging->rebuild);
                }
                restorePlugins();
                return ReloadOutcome::failed(error.what());
            }
        }
    }
    if (_staging->rebuild) {
        return ReloadOutcome::needsRebuild(*_staging->rebuild);
    }

    // Which files the reload touched, and what each one no longer declares.
    std::set<std::string> touched{file};
    for (const auto& [owner, declared] : _staging->declared) {
        touched.insert(owner);
    }
    std::vector<std::pair<std::string, ecs::Phase>> removedSystems;
    for (const std::string& owner : touched) {
        const FileDeclarations before = _files.contains(owner) ? _files.at(owner) : FileDeclarations{};
        const FileDeclarations after =
            _staging->declared.contains(owner) ? _staging->declared.at(owner) : FileDeclarations{};
        for (const std::string& component : before.components) {
            if (!after.components.contains(component)) {
                return ReloadOutcome::needsRebuild(
                    std::format("component '{}' is no longer declared by '{}'", component, owner));
            }
        }
        for (const auto& [name, phase] : before.systems) {
            if (!after.systems.contains(name)) {
                removedSystems.emplace_back(name, phase);
            }
        }
    }
    for (const system::BuiltSystem& built : _staging->systems) {
        const std::string name{built.spec.system.getName()};
        const std::optional<ecs::Phase> before = findSystemPhase(name);
        if (before && *before != built.phase) {
            return ReloadOutcome::needsRebuild(std::format("system '{}' moved from phase {} to phase {}", name,
                                                           ecs::toString(*before), ecs::toString(built.phase)));
        }
    }

    // Swapped one phase at a time: a phase is all or nothing.
    std::vector<std::string> what;
    std::array<std::vector<ecs::Schedule::SystemSpec>, ecs::kPhaseCount> put;
    std::array<std::vector<std::string>, ecs::kPhaseCount> removed;
    for (system::BuiltSystem& built : _staging->systems) {
        what.push_back(std::format("system {}", built.spec.system.getName()));
        put.at(static_cast<std::size_t>(built.phase)).push_back(std::move(built.spec));
    }
    for (const auto& [name, phase] : removedSystems) {
        what.push_back(std::format("removed system {}", name));
        removed.at(static_cast<std::size_t>(phase)).push_back(name);
    }
    bool swapped = false;
    for (std::size_t index = 0; index < ecs::kPhaseCount; ++index) {
        if (put.at(index).empty() && removed.at(index).empty()) {
            continue;
        }
        try {
            _app.getSchedules().replaceSystems(static_cast<ecs::Phase>(index), std::move(put.at(index)),
                                               removed.at(index));
            swapped = true;
        } catch (const ecs::exceptions::ScheduleException& error) {
            if (swapped) {
                return ReloadOutcome::needsRebuild(
                    std::format("part of the systems were replaced before an error: {}", error.what()));
            }
            restorePlugins();
            return ReloadOutcome::failed(error.what());
        }
    }

    // Done: the records follow the App.
    for (const std::string& owner : touched) {
        FileDeclarations after =
            _staging->declared.contains(owner) ? std::move(_staging->declared.at(owner)) : FileDeclarations{};
        for (const std::string& component : after.components) {
            what.push_back(std::format("component {}", component));
        }
        if (after.components.empty() && after.systems.empty()) {
            _files.erase(owner);
        } else {
            _files.insert_or_assign(owner, std::move(after));
        }
    }
    for (const std::string& name : _staging->declaredPlugins) {
        what.push_back(std::format("plugin {}", name));
    }
    // A plugin the script no longer declares goes with it.
    std::erase_if(_plugins, [&](const auto& entry) {
        return entry.second.file == file && !_staging->declaredPlugins.contains(entry.first);
    });
    return ReloadOutcome::applied(std::move(what));
}

luau::Result<> ScriptHost::registerAppType() {
    auto appType = _runtime.usertype<AppHandle>("App");
    if (!appType) {
        return tl::unexpected{std::move(appType.error())};
    }
    appType
        ->method("component",
                 [](const AppHandle& handle, std::string name, const luau::Table& spec) {
                     checkAlive(handle);
                     handle.host->declareComponent(std::move(name), spec);
                 })
        .method("system",
                [](const AppHandle& handle, std::string name, const luau::Table& spec) {
                    checkAlive(handle);
                    handle.host->declareSystem(std::move(name), spec);
                })
        .method("addPlugin",
                [](const AppHandle& handle, const std::string& name) {
                    checkAlive(handle);
                    handle.host->addPlugin(name);
                })
        .tostring([](const AppHandle& /*handle*/) { return std::string{"App"}; });
    return {};
}
}  // namespace rtype::scripting
