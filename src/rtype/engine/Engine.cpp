/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Engine
*/

#include "Engine.hpp"

#include <filesystem>
#include <format>
#include <stdexcept>

#include "ProjectSettings.hpp"
#include "ProjectSettingsRuntime.hpp"
#include "rtype/luau/Runtime.hpp"

namespace rtype::engine {
ProjectSettings Engine::loadProjectSettings(const std::filesystem::path& path) {
    ProjectSettingsRuntime runtime;
    const auto script = runtime.runtime().load(path);
    if (!script) {
        throw std::runtime_error{
            std::format("failed to load project settings at {}: {}", path.string(), script.error().message())};
    }
    if (const auto result = runtime.runtime().run(*script); !result) {
        throw std::runtime_error{
            std::format("failed to run project settings at {}: {}", path.string(), result.error().message())};
    }
    return runtime.result();
}
}  // namespace rtype::engine
