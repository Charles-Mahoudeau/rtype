/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginLoader
*/

#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "core/library/LibraryEntry.hpp"
#include "rtype/engine/Export.hpp"
#include "rtype/engine/plugin/SharedLibrary.hpp"

namespace rtype::engine::plugin {
/// @brief The plugin libraries of a program: opens those of a folder, checks them, and keeps them loaded.
///
/// @details It outlives every App built from the libraries, as their systems and hooks are code of the libraries. An
/// App built again after a reload registers the same libraries, which are not opened twice.
class RTYPE_ENGINE_API PluginLoader {
  public:
    PluginLoader() = default;
    ~PluginLoader() = default;
    PluginLoader(const PluginLoader& other) = delete;
    PluginLoader& operator=(const PluginLoader& other) = delete;
    PluginLoader(PluginLoader&& other) noexcept = default;
    PluginLoader& operator=(PluginLoader&& other) noexcept = default;

    /// @return The number of libraries loaded.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @brief Opens every shared library directly in @p directory, in the order of their names. A folder that does not
    /// exist has none. A folder given twice is read once.
    /// @throws exceptions::RuntimeError If a library cannot be opened, has no entry point, or has the file name of a
    /// library of another folder.
    /// @throws core::library::exceptions::LibraryException If a library was built for another engine.
    void load(const std::filesystem::path& directory);

    /// @brief Calls the registration of every library, in the order they were loaded.
    void registerInto(core::library::LibraryRegistrar& registrar) const;

  private:
    /// @brief One library, loaded and checked.
    struct Loaded {
        std::filesystem::path file;                ///< Its file.
        SharedLibrary library;                     ///< Keeps it in memory.
        const core::library::LibraryEntry* entry;  ///< What it exports.
    };

    /// @brief Opens and checks the library @p file.
    void loadFile(const std::filesystem::path& file);

    std::vector<std::filesystem::path> _directories;  ///< The folders read.
    std::vector<Loaded> _libraries;                   ///< The libraries, in the order loaded.
};
}  // namespace rtype::engine::plugin
