/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Registration
*/

#include "Registration.hpp"

#include <memory>

#include "SdlPlatform.hpp"
#include "core/config/Settings.hpp"
#include "rtype/contract/platform/IPlatform.hpp"
#include "rtype/contract/platform/PlatformRegistry.hpp"

namespace rtype::platform::sdl {

void registerPlatform(contract::platform::PlatformRegistry& registry) {
    registry.add("sdl", [](const core::config::Settings& settings) -> std::unique_ptr<contract::platform::IPlatform> {
        settings.checkKeys({}, "sdl");
        return std::make_unique<SdlPlatform>();
    });
}

}  // namespace rtype::platform::sdl
