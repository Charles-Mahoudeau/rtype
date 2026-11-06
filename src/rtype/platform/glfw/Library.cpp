/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include "core/library/LibraryEntry.hpp"
#include "rtype/contract/platform/PlatformRegistry.hpp"
#include "rtype/platform/glfw/Registration.hpp"

RTYPE_LIBRARY(registrar) {
    if (auto* registry = registrar.find<rtype::contract::platform::PlatformRegistry>(
            rtype::contract::platform::kPlatformRegistryService)) {
        rtype::platform::glfw::registerPlatform(*registry);
    }
}
