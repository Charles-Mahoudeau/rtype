/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include "backend/raylib/Registration.hpp"
#include "core/library/LibraryEntry.hpp"
#include "rtype/contract/platform/PlatformRegistry.hpp"
#include "rtype/contract/renderer/RendererRegistry.hpp"

RTYPE_LIBRARY(registrar) {
    if (auto* platforms = registrar.find<rtype::contract::platform::PlatformRegistry>(
            rtype::contract::platform::kPlatformRegistryService)) {
        rtype::backend::raylib::registerPlatform(*platforms);
    }
    if (auto* renderers = registrar.find<rtype::contract::renderer::RendererRegistry>(
            rtype::contract::renderer::kRendererRegistryService)) {
        rtype::backend::raylib::registerRenderer(*renderers);
    }
}
