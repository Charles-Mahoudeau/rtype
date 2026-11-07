/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleSettings
*/

#include "CapsuleSettings.hpp"

#include "rtype/core/config/Settings.hpp"

namespace rtype::plugins::capsule {
namespace {
#ifdef NDEBUG
constexpr bool kHotReloadByDefault = false;
#else
constexpr bool kHotReloadByDefault = true;
#endif
}  // namespace

CapsuleSettings CapsuleSettings::fromSettings(const core::config::Settings& settings) {
    const core::config::Settings section = settings.section("capsule");
    section.checkKeys({"hotReload"}, "capsule");
    return CapsuleSettings{.hotReload = section.getBool("hotReload", kHotReloadByDefault)};
}
}  // namespace rtype::plugins::capsule
