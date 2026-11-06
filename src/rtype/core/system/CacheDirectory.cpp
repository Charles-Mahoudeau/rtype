/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CacheDirectory
*/

#include "CacheDirectory.hpp"

#include <cstdlib>
#include <filesystem>

#ifdef _WIN32
#include <cwchar>
#include <memory>
#endif

namespace rtype::core::system {

namespace {

#ifndef _WIN32
/// @return The value of the environment variable @p name as an absolute path, or an empty path if it is unset, empty
/// or relative (the XDG specification says to ignore relative ones).
std::filesystem::path getAbsoluteEnvironmentPath(const char* name) {
    const char* value = std::getenv(name);
    if (value == nullptr) {
        return {};
    }
    const std::filesystem::path path{value};
    return path.is_absolute() ? path : std::filesystem::path{};
}
#endif

}  // namespace

std::filesystem::path getCacheDirectory() {
#ifdef _WIN32
    wchar_t* value = nullptr;
    std::size_t length = 0;
    // _wdupenv_s rather than _wgetenv, which MSVC deprecates: it copies the value, freed by the unique_ptr.
    if (_wdupenv_s(&value, &length, L"LOCALAPPDATA") != 0 || value == nullptr) {
        return {};
    }
    const std::unique_ptr<wchar_t, decltype(&std::free)> owned{value, &std::free};
    const std::filesystem::path path{owned.get()};
    return path.is_absolute() ? path : std::filesystem::path{};
#else
#ifndef __APPLE__
    if (std::filesystem::path xdg = getAbsoluteEnvironmentPath("XDG_CACHE_HOME"); !xdg.empty()) {
        return xdg;
    }
#endif
    const std::filesystem::path home = getAbsoluteEnvironmentPath("HOME");
    if (home.empty()) {
        return {};
    }
#ifdef __APPLE__
    return home / "Library" / "Caches";
#else
    return home / ".cache";
#endif
#endif
}

}  // namespace rtype::core::system
