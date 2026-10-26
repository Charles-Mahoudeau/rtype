/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Engine
*/

#pragma once

#include <filesystem>

#include "ProjectSettings.hpp"
#include "rtype/ecs/app/App.hpp"

namespace rtype::engine {
/// @brief Entry point of the engine: owns the application and loads the project settings.
class Engine {
  public:
    Engine() = default;
    ~Engine() = default;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    /// @brief Runs the project script and collects the settings it declares.
    /// @param path Path of the Luau project script.
    /// @return The settings set by the script.
    /// @throws std::runtime_error If the script cannot be loaded or run.
    [[nodiscard]] static ProjectSettings loadProjectSettings(const std::filesystem::path& path);

  private:
    ecs::App _app;  ///< The application the engine will drive.
};
}  // namespace rtype::engine
