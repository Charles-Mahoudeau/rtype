/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginLoader
*/

#include "PluginLoader.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "rtype/ecs/app/PluginEntry.hpp"
#include "rtype/engine/Exceptions.hpp"
#include "rtype/engine/plugin/SharedLibrary.hpp"

namespace rtype::engine::plugin {
namespace {
/// @brief What the entry point of a plugin library is.
using EntryPoint = const ecs::PluginLibrary* (*)();
}  // namespace

std::size_t PluginLoader::getCount() const noexcept { return _libraries.size(); }

void PluginLoader::load(const std::filesystem::path& directory) {
    if (std::ranges::find(_directories, directory) != _directories.end()) {
        return;
    }
    _directories.push_back(directory);
    std::vector<std::filesystem::path> files;
    std::error_code error;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator{directory, error}) {
        if (entry.path().extension() == SharedLibrary::getExtension()) {
            files.push_back(entry.path());
        }
    }
    std::ranges::sort(files);
    for (const std::filesystem::path& file : files) {
        loadFile(file);
    }
}

void PluginLoader::registerInto(ecs::PluginRegistrar& registrar) const {
    for (const Loaded& loaded : _libraries) {
        loaded.entry->registerPlugins(registrar);
    }
}

void PluginLoader::loadFile(const std::filesystem::path& file) {
    const auto known = std::ranges::find_if(
        _libraries, [&file](const Loaded& loaded) { return loaded.file.filename() == file.filename(); });
    if (known != _libraries.end()) {
        throw exceptions::RuntimeError{std::format("Two plugin libraries have the same name: '{}' and '{}'",
                                                   known->file.generic_string(), file.generic_string())};
    }
    SharedLibrary library{file};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): a symbol's address is what the system gives.
    const auto entryPoint = reinterpret_cast<EntryPoint>(library.findSymbol(ecs::kPluginEntrySymbol));
    if (entryPoint == nullptr) {
        throw exceptions::RuntimeError{std::format("'{}' is not a plugin library: it has no '{}'",
                                                   file.generic_string(), ecs::kPluginEntrySymbol)};
    }
    const ecs::PluginLibrary* entry = entryPoint();
    ecs::checkPluginLibrary(*entry, file.generic_string());
    _libraries.push_back(Loaded{.file = file, .library = std::move(library), .entry = entry});
}
}  // namespace rtype::engine::plugin
