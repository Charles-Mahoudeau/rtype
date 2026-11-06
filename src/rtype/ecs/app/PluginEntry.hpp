/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginEntry
*/

#pragma once

#include <string_view>

#include "core/library/LibraryEntry.hpp"
#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"

namespace rtype::ecs {

/// @brief The name the App's PluginRegistry is offered under to the libraries.
inline constexpr std::string_view kPluginRegistryService = "rtype.ecs.plugins";

/// @brief What a plugin library registers, given to its RTYPE_PLUGIN_LIBRARY body.
class RTYPE_ECS_API PluginRegistrar {
  public:
    /// @param library What the libraries exchange; it offers the PluginRegistry and must outlive the registrar.
    explicit PluginRegistrar(core::library::LibraryRegistrar& library) noexcept;
    ~PluginRegistrar() = default;
    PluginRegistrar(const PluginRegistrar& other) = delete;
    PluginRegistrar& operator=(const PluginRegistrar& other) = delete;
    PluginRegistrar(PluginRegistrar&& other) = delete;
    PluginRegistrar& operator=(PluginRegistrar&& other) = delete;

    /// @return Where the library's plugins go.
    [[nodiscard]] PluginRegistry& getPlugins() const noexcept;

    /// @return What the libraries exchange, for a plugin that offers a registry to its backends.
    [[nodiscard]] core::library::LibraryRegistrar& getLibrary() const noexcept;

  private:
    core::library::LibraryRegistrar* _library;  ///< What the libraries exchange.
};

}  // namespace rtype::ecs

/// @brief Makes a shared library a plugin library: put it once in a `.cpp`, followed by what the library registers.
/// @code
/// RTYPE_PLUGIN_LIBRARY(registrar) {
///     registrar.getPlugins().add("game.Boss", [](const rtype::core::config::Settings& settings) {
///         return std::make_unique<BossPlugin>(settings);
///     });
/// }
/// @endcode
// NOLINTBEGIN(cppcoreguidelines-macro-usage,bugprone-macro-parentheses): it wraps RTYPE_LIBRARY and names a parameter.
#define RTYPE_PLUGIN_LIBRARY(target)                                         \
    static void rtypeRegisterPlugins(::rtype::ecs::PluginRegistrar& target); \
    RTYPE_LIBRARY(rtypeLibraryRegistrar) {                                   \
        ::rtype::ecs::PluginRegistrar plugins{rtypeLibraryRegistrar};        \
        rtypeRegisterPlugins(plugins);                                       \
    }                                                                        \
    static void rtypeRegisterPlugins(::rtype::ecs::PluginRegistrar& target)
// NOLINTEND(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)
