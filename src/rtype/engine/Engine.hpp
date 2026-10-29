/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Engine
*/

#pragma once

#include "Export.hpp"
#include "rtype/ecs/app/App.hpp"

namespace rtype::engine {
/// @brief Entry point of the engine: owns the application and loads the project settings.
class RTYPE_ENGINE_API Engine {
  public:
    Engine() = default;
    ~Engine() = default;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

  private:
    ecs::App _app;  ///< The application the engine will drive.
};
}  // namespace rtype::engine
