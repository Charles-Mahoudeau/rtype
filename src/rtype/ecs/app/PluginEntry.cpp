/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginEntry
*/

#include "PluginEntry.hpp"

#include "core/library/LibraryEntry.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"

namespace rtype::ecs {
PluginRegistrar::PluginRegistrar(core::library::LibraryRegistrar& library) noexcept : _library{&library} {}

PluginRegistry& PluginRegistrar::getPlugins() const noexcept {
    return *_library->find<PluginRegistry>(kPluginRegistryService);
}

core::library::LibraryRegistrar& PluginRegistrar::getLibrary() const noexcept { return *_library; }
}  // namespace rtype::ecs
