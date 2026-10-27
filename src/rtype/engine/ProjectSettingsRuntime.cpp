/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ProjectSettingsRuntime
*/

#include "ProjectSettingsRuntime.hpp"

#include <format>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "ProjectSettings.hpp"
#include "rtype/luau/Library.hpp"
#include "rtype/luau/LibraryBuilder.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/RuntimeConfig.hpp"

namespace {
/// @brief Configuration of the settings runtime: no standard library, no optimization.
constexpr rtype::luau::RuntimeConfig kConfig{
    .libs = rtype::luau::RuntimeConfig::Libs::kNone,
    .optimizationLevel = rtype::luau::RuntimeConfig::OptimizationLevel::kDisabled,
};

/// @return A runtime built from `kConfig`.
/// @throws std::runtime_error If the runtime cannot be created.
rtype::luau::Runtime createRuntime() {
    auto runtime = rtype::luau::Runtime::create(kConfig);
    if (!runtime) {
        throw std::runtime_error{
            std::format("Failed to create project settings runtime: {}", runtime.error().message())};
    }
    return std::move(*runtime);
}
}  // namespace

namespace rtype::engine {
ProjectSettingsRuntime::ProjectSettingsRuntime() : _runtime{createRuntime()}, _library{buildLibrary()} {
    const auto result = _runtime.globals().registerLibrary(_library);
    if (!result) {
        throw std::runtime_error{std::format(
            "Failed to register the engine library in the project settings runtime: {}", result.error().message())};
    }
}

luau::Runtime& ProjectSettingsRuntime::runtime() { return _runtime; }

ProjectSettings ProjectSettingsRuntime::result() const { return *_result; }

luau::Library ProjectSettingsRuntime::buildLibrary() {
    luau::LibraryBuilder builder{_runtime};
    builder.setName("engine");
    builder.adoptLibrary(buildSettingsLibrary());
    return builder.build();
}

luau::Library ProjectSettingsRuntime::buildSettingsLibrary() {
    luau::LibraryBuilder builder{_runtime};
    builder.setName("settings");
    builder.function("setGameName").bind([result = _result](std::string name) {
        std::cout << "[ProjectSettingsRuntime] game name set to: " << name << std::endl;
        result->gameName = std::move(name);
        return luau::ValuePack{};
    });
    builder.function("setGameVersion").bind([result = _result](std::string name) {
        std::cout << "[ProjectSettingsRuntime] game version set to: " << name << std::endl;
        result->gameVersion = std::move(name);
        return luau::ValuePack{};
    });
    builder.function("setPlatform").bind([result = _result](std::string name) {
        std::cout << "[ProjectSettingsRuntime] platform set to: " << name << std::endl;
        result->platform = std::move(name);
        return luau::ValuePack{};
    });
    builder.function("setRenderer").bind([result = _result](std::string name) {
        std::cout << "[ProjectSettingsRuntime] renderer set to: " << name << std::endl;
        result->renderer = std::move(name);
        return luau::ValuePack{};
    });
    return builder.build();
}
}  // namespace rtype::engine
