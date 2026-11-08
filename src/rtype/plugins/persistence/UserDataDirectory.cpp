/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UserDataDirectory
*/

#include "plugins/persistence/UserDataDirectory.hpp"

#include <cstdlib>
#include <filesystem>
#include <memory>  // IWYU pragma: keep (std::unique_ptr, used only on Windows)
#include <string>  // IWYU pragma: keep (used only on Windows)

namespace rtype::plugins::persistence {
namespace {
/// @return The absolute path in the environment variable @p name, or an empty path if unset, empty or relative.
std::filesystem::path absoluteEnvironmentPath(const char* name) {
#ifdef _WIN32
    char* value = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&value, &length, name) != 0 || value == nullptr) {
        return {};
    }
    const std::unique_ptr<char, decltype(&std::free)> owned{value, &std::free};
    const std::filesystem::path path{std::string{owned.get()}};
#else
    const char* value = std::getenv(name);
    if (value == nullptr) {
        return {};
    }
    const std::filesystem::path path{value};
#endif
    return path.is_absolute() ? path : std::filesystem::path{};
}
}  // namespace

std::filesystem::path getUserDataDirectory() {
#ifdef _WIN32
    return absoluteEnvironmentPath("APPDATA");
// NOLINTNEXTLINE(readability-use-concise-preprocessor-directives): #elifdef is not in every compiler the project needs.
#elif defined(__APPLE__)
    const std::filesystem::path home = absoluteEnvironmentPath("HOME");
    return home.empty() ? home : home / "Library" / "Application Support";
#else
    if (std::filesystem::path xdg = absoluteEnvironmentPath("XDG_DATA_HOME"); !xdg.empty()) {
        return xdg;
    }
    const std::filesystem::path home = absoluteEnvironmentPath("HOME");
    return home.empty() ? home : home / ".local" / "share";
#endif
}

}  // namespace rtype::plugins::persistence
