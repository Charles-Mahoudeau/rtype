/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MinimalPlugins
*/

#include "MinimalPlugins.hpp"

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"

namespace rtype::ecs {
void MinimalPlugins::build(App& app) { app.addPlugin(TimePlugin{}); }
}  // namespace rtype::ecs
