/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>

#include "backend/null/NullBackend.hpp"
#include "core/config/Settings.hpp"
#include "core/library/LibraryEntry.hpp"
#include "rtype/contract/platform/IPlatform.hpp"
#include "rtype/contract/platform/PlatformRegistry.hpp"
#include "rtype/contract/renderer/IRenderer.hpp"
#include "rtype/contract/renderer/RendererRegistry.hpp"

RTYPE_LIBRARY(registrar) {
    using namespace rtype;
    if (auto* platforms =
            registrar.find<contract::platform::PlatformRegistry>(contract::platform::kPlatformRegistryService)) {
        platforms->add("null",
                       [](const core::config::Settings& settings) -> std::unique_ptr<contract::platform::IPlatform> {
                           settings.checkKeys({}, "null");
                           return std::make_unique<backend::null::NullPlatform>();
                       });
    }
    if (auto* renderers =
            registrar.find<contract::renderer::RendererRegistry>(contract::renderer::kRendererRegistryService)) {
        renderers->add("null",
                       [](const core::config::Settings& settings) -> std::unique_ptr<contract::renderer::IRenderer> {
                           settings.checkKeys({}, "null");
                           return std::make_unique<backend::null::NullRenderer>();
                       });
    }
}
