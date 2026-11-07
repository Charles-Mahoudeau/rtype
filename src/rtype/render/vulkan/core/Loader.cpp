/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Loader
*/

#include "Loader.hpp"

#include <array>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#ifdef __APPLE__
#include <vulkan/vulkan.hpp>
#else
#include <dlfcn.h>
#endif
#endif

namespace rtype::render::vulkan::core {

#ifdef __APPLE__

Loader::Loader() : _entryPoint{vkGetInstanceProcAddr} {}

#else

namespace {

#ifdef _WIN32
constexpr std::array<std::string_view, 1> kLibraryNames = {"vulkan-1.dll"};
#else
constexpr std::array<std::string_view, 2> kLibraryNames = {"libvulkan.so.1", "libvulkan.so"};
#endif

/// @return The vkGetInstanceProcAddr of the first loader library that opens, or null. The library stays open.
PFN_vkGetInstanceProcAddr openEntryPoint() {
    for (const std::string_view name : kLibraryNames) {
        const std::string file{name};
#ifdef _WIN32
        const HMODULE library = LoadLibraryA(file.c_str());
        if (library == nullptr) {
            continue;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): a symbol of a library is a plain address.
        void* const symbol = reinterpret_cast<void*>(GetProcAddress(library, "vkGetInstanceProcAddr"));
#else
        void* const library = dlopen(file.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (library == nullptr) {
            continue;
        }
        void* const symbol = dlsym(library, "vkGetInstanceProcAddr");
#endif
        if (symbol != nullptr) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): a symbol of a library is a plain address.
            return reinterpret_cast<PFN_vkGetInstanceProcAddr>(symbol);
        }
    }
    return nullptr;
}

}  // namespace

Loader::Loader() : _entryPoint{openEntryPoint()} {
    if (_entryPoint == nullptr) {
        std::string tried;
        for (const std::string_view name : kLibraryNames) {
            tried += tried.empty() ? std::string{name} : std::format(", {}", name);
        }
        throw std::runtime_error(
            std::format("Vulkan loader not found (tried {}): install the driver of your graphics card", tried));
    }
}

#endif

PFN_vkGetInstanceProcAddr Loader::getEntryPoint() const noexcept { return _entryPoint; }

}  // namespace rtype::render::vulkan::core
