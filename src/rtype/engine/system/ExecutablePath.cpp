/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ExecutablePath
*/

#include "ExecutablePath.hpp"

#include <filesystem>
#include <format>
#include <stdexcept>
#include <string>
#include <system_error>

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
#include <mach-o/dyld.h>

#include <cstdint>
#endif
#endif

namespace rtype::engine::system {

namespace {

/// @return The path the operating system reports for the running executable, possibly not canonical.
/// @throws std::runtime_error If it cannot be obtained.
std::filesystem::path queryExecutablePath() {
#ifdef _WIN32
    std::wstring buffer(MAX_PATH, L'\0');
    while (true) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            throw std::runtime_error("Cannot get the executable path: GetModuleFileNameW failed");
        }
        if (length < buffer.size()) {
            buffer.resize(length);
            return std::filesystem::path{buffer};
        }
        buffer.resize(buffer.size() * 2);
    }
#else
#ifdef __APPLE__
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string buffer(size, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        throw std::runtime_error("Cannot get the executable path: _NSGetExecutablePath failed");
    }
    buffer.resize(buffer.find('\0'));
    return std::filesystem::path{buffer};
#else
    std::error_code error;
    std::filesystem::path path = std::filesystem::read_symlink("/proc/self/exe", error);
    if (error) {
        throw std::runtime_error(std::format("Cannot get the executable path: /proc/self/exe: {}", error.message()));
    }
    return path;
#endif
#endif
}

}  // namespace

std::filesystem::path getExecutablePath() {
    std::error_code error;
    const std::filesystem::path canonical = std::filesystem::weakly_canonical(queryExecutablePath(), error);
    if (error) {
        throw std::runtime_error(std::format("Cannot resolve the executable path: {}", error.message()));
    }
    return canonical;
}

std::filesystem::path getExecutableDirectory() { return getExecutablePath().parent_path(); }

}  // namespace rtype::engine::system
