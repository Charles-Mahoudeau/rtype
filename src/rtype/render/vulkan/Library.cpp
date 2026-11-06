/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include "core/library/LibraryEntry.hpp"
#include "rtype/contract/renderer/RendererRegistry.hpp"
#include "rtype/render/vulkan/Registration.hpp"

RTYPE_LIBRARY(registrar) {
    if (auto* registry = registrar.find<rtype::contract::renderer::RendererRegistry>(
            rtype::contract::renderer::kRendererRegistryService)) {
        rtype::render::vulkan::registerRenderer(*registry);
    }
}
