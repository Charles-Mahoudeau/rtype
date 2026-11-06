/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginEntry
*/

#pragma once

#include <cstdint>
#include <string_view>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"

/// @brief Names the compiler and the standard library a binary was built with, as a string literal.
/// @details The interface of a plugin is C++: a library and the engine must agree on it, so they must be built alike.
#define RTYPE_ECS_DETAIL_STRINGIFY_VALUE(value) #value
#define RTYPE_ECS_DETAIL_STRINGIFY(value) RTYPE_ECS_DETAIL_STRINGIFY_VALUE(value)
#if defined(_MSC_VER) && !defined(__clang__)
#define RTYPE_ECS_DETAIL_COMPILER "msvc " RTYPE_ECS_DETAIL_STRINGIFY(_MSC_VER)
// NOLINTNEXTLINE(readability-use-concise-preprocessor-directives): #elifdef is not in every compiler the project needs.
#elif defined(__clang__)
#define RTYPE_ECS_DETAIL_COMPILER "clang " __clang_version__
#else
#define RTYPE_ECS_DETAIL_COMPILER "gcc " __VERSION__
#endif
#ifdef _ITERATOR_DEBUG_LEVEL
#define RTYPE_ECS_DETAIL_LIBRARY " iterators " RTYPE_ECS_DETAIL_STRINGIFY(_ITERATOR_DEBUG_LEVEL)
#else
#define RTYPE_ECS_DETAIL_LIBRARY ""
#endif
#define RTYPE_ECS_PLUGIN_TOOLCHAIN RTYPE_ECS_DETAIL_COMPILER RTYPE_ECS_DETAIL_LIBRARY

#ifdef _WIN32
#define RTYPE_ECS_PLUGIN_ENTRY_EXPORT __declspec(dllexport)
#else
#define RTYPE_ECS_PLUGIN_ENTRY_EXPORT __attribute__((visibility("default")))
#endif

namespace rtype::ecs {

/// @brief The version of what a plugin library and the engine share: raised when PluginLibrary or PluginRegistrar
/// change, so an older library is refused rather than called wrongly.
inline constexpr std::uint32_t kPluginAbiVersion = 1;

/// @brief The name of the C function a plugin library exports (RTYPE_PLUGIN_LIBRARY).
inline constexpr std::string_view kPluginEntrySymbol = "rtypePluginLibrary";

/// @brief What a plugin library registers, given to its RTYPE_PLUGIN_LIBRARY body.
class RTYPE_ECS_API PluginRegistrar {
  public:
    /// @param plugins Where the library's plugins go; it must outlive the registrar.
    explicit PluginRegistrar(PluginRegistry& plugins) noexcept;
    ~PluginRegistrar() = default;
    PluginRegistrar(const PluginRegistrar& other) = delete;
    PluginRegistrar& operator=(const PluginRegistrar& other) = delete;
    PluginRegistrar(PluginRegistrar&& other) = delete;
    PluginRegistrar& operator=(PluginRegistrar&& other) = delete;

    /// @return Where the library's plugins go.
    [[nodiscard]] PluginRegistry& getPlugins() noexcept;

  private:
    PluginRegistry* _plugins;  ///< Where the library's plugins go.
};

/// @brief What a plugin library gives the engine: plain data with C linkage, the only part of the interface that does
/// not depend on how the library was built. Its first two fields never move, so any library can be checked.
struct PluginLibrary {
    std::uint32_t abiVersion;                          ///< kPluginAbiVersion, when the library was built.
    const char* toolchain;                             ///< RTYPE_ECS_PLUGIN_TOOLCHAIN, when the library was built.
    void (*registerPlugins)(PluginRegistrar& target);  ///< Registers what the library brings.
};

/// @brief Refuses a library the engine cannot call.
/// @param library What the library exports.
/// @param file The library's file, for the message.
/// @throws exceptions::PluginException If its version or its toolchain differs from the engine's, naming both.
RTYPE_ECS_API void checkPluginLibrary(const PluginLibrary& library, std::string_view file);

}  // namespace rtype::ecs

/// @brief Makes a shared library a plugin library: put it once in a `.cpp`, followed by what the library registers.
/// @code
/// RTYPE_PLUGIN_LIBRARY(registrar) {
///     registrar.getPlugins().add("game.Boss", [](const rtype::core::config::Settings& settings) {
///         return std::make_unique<BossPlugin>(settings);
///     });
/// }
/// @endcode
// NOLINTBEGIN(cppcoreguidelines-macro-usage,bugprone-macro-parentheses): it declares an exported C function and names
// a parameter, which only a macro can do and which parentheses would break.
#define RTYPE_PLUGIN_LIBRARY(target)                                                                                  \
    static void rtypeRegisterPlugins(::rtype::ecs::PluginRegistrar& target);                                          \
    extern "C" RTYPE_ECS_PLUGIN_ENTRY_EXPORT const ::rtype::ecs::PluginLibrary* rtypePluginLibrary() {                \
        static const ::rtype::ecs::PluginLibrary library{::rtype::ecs::kPluginAbiVersion, RTYPE_ECS_PLUGIN_TOOLCHAIN, \
                                                         &rtypeRegisterPlugins};                                      \
        return &library;                                                                                              \
    }                                                                                                                 \
    static void rtypeRegisterPlugins(::rtype::ecs::PluginRegistrar& target)
// NOLINTEND(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)
