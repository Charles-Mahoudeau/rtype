/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptHost
*/

#include "ScriptHost.hpp"

#include <algorithm>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>

#include "commands/ScriptCommands.hpp"
#include "component/ComponentLayout.hpp"
#include "component/ComponentRegistration.hpp"
#include "component/DeclarationReader.hpp"
#include "plugin/PluginDeclarationReader.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/Function.hpp"
#include "rtype/luau/LibraryBuilder.hpp"
#include "rtype/luau/Result.hpp"
#include "rtype/luau/RuntimeConfig.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Userdata.hpp"
#include "rtype/luau/Usertype.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"
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

luau::Result<std::unique_ptr<ScriptHost>> ScriptHost::create(ecs::App& app, const Options options) {
    auto runtime = luau::Runtime::create(makeRuntimeConfig());
    if (!runtime) {
        return tl::unexpected{std::move(runtime.error())};
    }
    // The constructor is private, so make_unique cannot reach it.
    std::unique_ptr<ScriptHost> host{new ScriptHost{app, std::move(*runtime)}};
    if (auto installed = host->installEngine(options); !installed) {
        return tl::unexpected{std::move(installed.error())};
    }
    return host;
}

ScriptHost::ScriptHost(ecs::App& app, luau::Runtime runtime) noexcept
    : _app{app}, _runtime{std::move(runtime)}, _handle{std::make_shared<system::RuntimeHandle>()} {
    _handle->runtime = &_runtime;
    _handle->components = &_components;
}

ScriptHost::~ScriptHost() { _handle->runtime = nullptr; }

luau::Result<> ScriptHost::run(std::string name, const std::string& source) const {
    auto script = _runtime.load(std::move(name), source);
    if (!script) {
        return tl::unexpected{std::move(script.error())};
    }
    return _runtime.run(*script);
}

const component::ComponentLayout* ScriptHost::findComponent(const std::string_view name) const noexcept {
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
    ecsBuilder.function("plugin").bind([this](std::string name, const luau::Table& spec) {
        declarePlugin(std::move(name), spec);
        return luau::ValuePack{};
    });
    ecsBuilder.function("addPlugin").bind([this](const std::string& name) {
        addPlugin(name);
        return luau::ValuePack{};
    });

    luau::LibraryBuilder engineBuilder{_runtime};
    engineBuilder.setName("engine");
    engineBuilder.adoptLibrary(ecsBuilder.build());
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
    return _runtime.globals().registerLibrary(*_engine);
}

void ScriptHost::declareComponent(std::string name, const luau::Table& spec) {
    const auto declaration = component::readComponentDeclaration(std::move(name), spec);
    if (!declaration) {
        throw luau::exceptions::RuntimeError{std::string{declaration.error().message()}};
    }
    auto layout = component::registerComponent(_app.getWorld().getComponents(), *declaration);
    if (!layout) {
        throw luau::exceptions::RuntimeError{std::string{layout.error().message()}};
    }
    _components.insert_or_assign(declaration->name, std::move(*layout));
}

void ScriptHost::declareSystem(std::string name, const luau::Table& spec) {
    auto declaration = system::readSystemDeclaration(std::move(name), spec);
    if (!declaration) {
        throw luau::exceptions::RuntimeError{std::string{declaration.error().message()}};
    }
    if (auto registered = system::registerSystem(_app, std::move(*declaration), _handle); !registered) {
        throw luau::exceptions::RuntimeError{std::string{registered.error().message()}};
    }
}

void ScriptHost::declarePlugin(std::string name, const luau::Table& spec) {
    auto declaration = plugin::readPluginDeclaration(std::move(name), spec);
    if (!declaration) {
        throw luau::exceptions::RuntimeError{std::string{declaration.error().message()}};
    }
    if (_plugins.contains(declaration->name)) {
        throw luau::exceptions::RuntimeError{std::format("Plugin '{}': already declared", declaration->name)};
    }
    const std::string key = declaration->name;
    _plugins.emplace(key, PluginEntry{.declaration = std::move(*declaration)});
}

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
            auto handle = std::make_shared<bool>(true);
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
