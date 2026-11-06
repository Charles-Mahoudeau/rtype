/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** LibraryEntry
*/

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>

/// @brief Names the compiler and the standard library a binary was built with, as a string literal.
/// @details What a library shares with the engine is C++: they must agree on it, so they must be built alike.
// NOLINTBEGIN(cppcoreguidelines-macro-usage): these build a string literal from the compiler's own macros.
#define RTYPE_CORE_DETAIL_STRINGIFY_VALUE(value) #value
#define RTYPE_CORE_DETAIL_STRINGIFY(value) RTYPE_CORE_DETAIL_STRINGIFY_VALUE(value)
#if defined(_MSC_VER) && !defined(__clang__)
#define RTYPE_CORE_DETAIL_COMPILER "msvc " RTYPE_CORE_DETAIL_STRINGIFY(_MSC_VER)
// NOLINTNEXTLINE(readability-use-concise-preprocessor-directives): #elifdef is not in every compiler the project needs.
#elif defined(__clang__)
#define RTYPE_CORE_DETAIL_COMPILER "clang " __clang_version__
#else
#define RTYPE_CORE_DETAIL_COMPILER "gcc " __VERSION__
#endif
#ifdef _ITERATOR_DEBUG_LEVEL
#define RTYPE_CORE_DETAIL_STANDARD_LIBRARY " iterators " RTYPE_CORE_DETAIL_STRINGIFY(_ITERATOR_DEBUG_LEVEL)
#else
#define RTYPE_CORE_DETAIL_STANDARD_LIBRARY ""
#endif
#define RTYPE_LIBRARY_TOOLCHAIN RTYPE_CORE_DETAIL_COMPILER RTYPE_CORE_DETAIL_STANDARD_LIBRARY

#ifdef _WIN32
#define RTYPE_LIBRARY_EXPORT __declspec(dllexport)
#else
#define RTYPE_LIBRARY_EXPORT __attribute__((visibility("default")))
#endif
// NOLINTEND(cppcoreguidelines-macro-usage)

namespace rtype::core::library {

/// @brief The version of what a library and the engine share: raised when LibraryEntry or LibraryRegistrar change, so
/// an older library is refused rather than called wrongly.
inline constexpr std::uint32_t kLibraryAbiVersion = 1;

/// @brief The name of the C function a library exports (RTYPE_LIBRARY).
inline constexpr std::string_view kLibraryEntrySymbol = "rtypeLibrary";

/// @brief What the libraries of a program exchange while they register: the registries one offers and others fill, by
/// name, so a backend fills the renderers of the graphics plugin without knowing it, or the ECS.
class LibraryRegistrar {
  public:
    LibraryRegistrar() = default;
    ~LibraryRegistrar() = default;
    LibraryRegistrar(const LibraryRegistrar& other) = delete;
    LibraryRegistrar& operator=(const LibraryRegistrar& other) = delete;
    LibraryRegistrar(LibraryRegistrar&& other) = delete;
    LibraryRegistrar& operator=(LibraryRegistrar&& other) = delete;

    /// @brief Offers @p service to the libraries registered after this one, replacing one of the same name.
    /// @param name What it is known by; every user of the name agrees on @p T.
    /// @param service Must outlive the registrar.
    template <typename T>
    void provide(std::string_view name, T& service);

    /// @return The service @p name, or nullptr if no library offers it.
    template <typename T>
    [[nodiscard]] T* find(std::string_view name) const;

  private:
    std::map<std::string, void*, std::less<>> _services;  ///< The services by name.
};

/// @brief What a library gives the engine: plain data with C linkage, the only part of the interface that does not
/// depend on how the library was built. Its first two fields never move, so any library can be checked.
struct LibraryEntry {
    std::uint32_t abiVersion;                           ///< kLibraryAbiVersion, when the library was built.
    const char* toolchain;                              ///< RTYPE_LIBRARY_TOOLCHAIN, when the library was built.
    void (*registerLibrary)(LibraryRegistrar& target);  ///< Registers what the library brings.
};

/// @brief Refuses a library the engine cannot call.
/// @param entry What the library exports.
/// @param file The library's file, for the message.
/// @throws exceptions::LibraryException If its version or its toolchain differs from the engine's, naming both.
void checkLibrary(const LibraryEntry& entry, std::string_view file);

}  // namespace rtype::core::library

/// @brief Makes a shared library loadable by the engine: put it once in a `.cpp`, followed by what it registers. A
/// plugin library uses RTYPE_PLUGIN_LIBRARY instead.
/// @details The function it declares has internal linkage: every library has one of this name, and on Linux a visible
/// one would resolve to the first library loaded.
/// @code
/// RTYPE_LIBRARY(registrar) {
///     if (auto* renderers = registrar.find<RendererRegistry>(kRendererRegistryService)) {
///         registerRenderer(*renderers);
///     }
/// }
/// @endcode
// NOLINTBEGIN(cppcoreguidelines-macro-usage,bugprone-macro-parentheses): it declares an exported C function and names
// a parameter, which only a macro can do and which parentheses would break.
#define RTYPE_LIBRARY(target)                                                                                      \
    namespace {                                                                                                    \
    namespace rtype_library {                                                                                      \
    void registerLibrary(::rtype::core::library::LibraryRegistrar& target);                                        \
    }                                                                                                              \
    }                                                                                                              \
    extern "C" RTYPE_LIBRARY_EXPORT const ::rtype::core::library::LibraryEntry* rtypeLibrary() {                   \
        static const ::rtype::core::library::LibraryEntry entry{                                                   \
            ::rtype::core::library::kLibraryAbiVersion, RTYPE_LIBRARY_TOOLCHAIN, &rtype_library::registerLibrary}; \
        return &entry;                                                                                             \
    }                                                                                                              \
    void rtype_library::registerLibrary(::rtype::core::library::LibraryRegistrar& target)
// NOLINTEND(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)

#define RTYPE_LIBRARY_ENTRY_INCLUDING_TPP
#include "LibraryEntry.tpp"  // IWYU pragma: export
#undef RTYPE_LIBRARY_ENTRY_INCLUDING_TPP
