/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <string>

#include "plugins/sprite/SpriteCalls.hpp"
#include "plugins/sprite/SpriteDrawPlugin.hpp"
#include "plugins/sprite/SpritePlugin.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    rtype::plugins::sprite::addSpriteForms(registrar.getPlugins());
    using rtype::plugins::sprite::SpriteDrawPlugin;
    using rtype::plugins::sprite::SpritePlugin;
    registrar.getPlugins().add(std::string{SpritePlugin::kName}, [] { return std::make_unique<SpritePlugin>(); });
    registrar.getPlugins().add(std::string{SpriteDrawPlugin::kName},
                               [] { return std::make_unique<SpriteDrawPlugin>(); });
}
