/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptHost
*/

#include "ScriptHost.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>

#include "component/ComponentLayout.hpp"
#include "component/ComponentRegistration.hpp"
#include "component/DeclarationReader.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/LibraryBuilder.hpp"
#include "rtype/luau/Result.hpp"
#include "rtype/luau/RuntimeConfig.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"
#include "system/SystemDeclarationReader.hpp"
#include "system/SystemRegistration.hpp"

namespace rtype::scripting {
namespace {
/// @return The configuration of the scripts' runtime: the standard libraries (the runtime opens all of them or none).
luau::RuntimeConfig makeRuntimeConfig() {
    return {
        .libs = luau::RuntimeConfig::Libs::kStandard,
        .optimizationLevel = luau::RuntimeConfig::OptimizationLevel::kStandard,
    };
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

ScriptHost::ScriptHost(ecs::App& app, luau::Runtime runtime) noexcept : _app{app}, _runtime{std::move(runtime)} {}

ScriptHost::~ScriptHost() = default;

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
    if (auto registered = system::registerSystem(_app, std::move(*declaration)); !registered) {
        throw luau::exceptions::RuntimeError{std::string{registered.error().message()}};
    }
}
}  // namespace rtype::scripting
