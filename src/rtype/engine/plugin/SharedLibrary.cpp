/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SharedLibrary
*/

#include "SharedLibrary.hpp"

#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <utility>

#include "rtype/engine/Exceptions.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace rtype::engine::plugin {
namespace {
#ifdef _WIN32
constexpr std::string_view kExtension = ".dll";
// NOLINTNEXTLINE(readability-use-concise-preprocessor-directives): #elifdef is not in every compiler the project needs.
#elif defined(__APPLE__)
constexpr std::string_view kExtension = ".dylib";
#else
constexpr std::string_view kExtension = ".so";
#endif
}  // namespace

// NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer): it is set by a different call on each system.
SharedLibrary::SharedLibrary(const std::filesystem::path& path) {
#ifdef _WIN32
    // A module of the same name is shared: the engine's own copy, next to the executable, is the one its code uses.
    _handle = GetModuleHandleW(path.filename().c_str());
    if (_handle != nullptr) {
        return;
    }
    // What the library links is looked for next to it, then in the usual places.
    _handle = LoadLibraryExW(std::filesystem::absolute(path).c_str(), nullptr,
                             LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (_handle == nullptr) {
        throw exceptions::RuntimeError{
            std::format("Cannot load the library '{}': system error {}", path.generic_string(), GetLastError())};
    }
#else
    _handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (_handle == nullptr) {
        const char* reason = dlerror();
        throw exceptions::RuntimeError{std::format("Cannot load the library '{}': {}", path.generic_string(),
                                                   reason != nullptr ? reason : "unknown error")};
    }
#endif
    _owned = true;
}

SharedLibrary::~SharedLibrary() { close(); }

SharedLibrary::SharedLibrary(SharedLibrary&& other) noexcept
    : _handle{std::exchange(other._handle, nullptr)}, _owned{std::exchange(other._owned, false)} {}

SharedLibrary& SharedLibrary::operator=(SharedLibrary&& other) noexcept {
    if (this != &other) {
        close();
        _handle = std::exchange(other._handle, nullptr);
        _owned = std::exchange(other._owned, false);
    }
    return *this;
}

void* SharedLibrary::findSymbol(const std::string_view name) const {
    const std::string symbol{name};
#ifdef _WIN32
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): the system gives a function, callers cast it back.
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(_handle), symbol.c_str()));
#else
    return dlsym(_handle, symbol.c_str());
#endif
}

std::string_view SharedLibrary::getExtension() noexcept { return kExtension; }

void SharedLibrary::close() noexcept {
    if (_handle != nullptr && _owned) {
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(_handle));
#else
        dlclose(_handle);
#endif
    }
    _handle = nullptr;
    _owned = false;
}
}  // namespace rtype::engine::plugin
