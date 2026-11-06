/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderConfig
*/

#include "plugins/shader/ShaderConfig.hpp"

#include <filesystem>

#include "rtype/core/config/Settings.hpp"

namespace rtype::plugins::shader {

namespace {
#ifdef NDEBUG
constexpr bool kHotReloadByDefault = false;
#else
constexpr bool kHotReloadByDefault = true;
#endif
}  // namespace

ShaderConfig ShaderConfig::fromSettings(const core::config::Settings& settings) {
    const core::config::Settings section = settings.section("shader");
    section.checkKeys({"hotReload"}, "shader");
    return ShaderConfig{.gameDirectory = std::filesystem::path{settings.getString(kGameDirectoryKey, "")},
                        .hotReload = section.getBool("hotReload", kHotReloadByDefault)};
}

}  // namespace rtype::plugins::shader
