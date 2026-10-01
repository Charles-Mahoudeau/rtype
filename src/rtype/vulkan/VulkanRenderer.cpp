/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** VulkanRenderer
*/

#include "VulkanRenderer.hpp"

#include <bit>
#include <cstddef>
#include <glm/ext/vector_uint2.hpp>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "core/DebugMessenger.hpp"
#include "core/Instance.hpp"
#include "engine/exceptions/PlatformExceptions.hpp"
#include "engine/graphics/Camera.hpp"
#include "engine/graphics/Color.hpp"
#include "engine/graphics/RectShape.hpp"
#include "engine/graphics/Sprite.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/platform/IPlatform.hpp"

namespace rtype::vulkan {

namespace {
constexpr std::string_view kEngineName = "R-Type Engine";
constexpr const char* kValidationLayer = "VK_LAYER_KHRONOS_validation";
}  // namespace

VulkanRenderer::VulkanRenderer(bool enableValidation) : _enableValidation(enableValidation) {}

engine::platform::ProcAddress VulkanRenderer::getLoaderEntryPoint() const {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<engine::platform::ProcAddress>(core::Instance::getLoaderEntryPoint());
}

void VulkanRenderer::init(engine::platform::IPlatform& platform) {
    if (_instance.has_value()) {
        throw std::runtime_error("VulkanRenderer::init() called twice");
    }

    std::vector<const char*> extensions = platform.getRequiredExtensions();
    std::vector<const char*> layers;
    if (_enableValidation) {
        layers.push_back(kValidationLayer);
        extensions.push_back(core::DebugMessenger::kExtensionName);
    }
    _instance.emplace(platform.getTitle(), kEngineName, VK_API_VERSION_1_3, std::move(extensions), std::move(layers));
    if (_enableValidation) {
        _debugMessenger.emplace(*_instance);
    }

    auto* const instance = static_cast<VkInstance>(*_instance->getInstance());
    _surface =
        vk::raii::SurfaceKHR(_instance->getInstance(), std::bit_cast<VkSurfaceKHR>(platform.createSurface(instance)));
    _framebufferSize = platform.getFramebufferSize();
}

void VulkanRenderer::resize(glm::uvec2 framebufferSize) { _framebufferSize = framebufferSize; }

engine::graphics::TextureId VulkanRenderer::createTexture(const engine::graphics::TextureDesc& /*desc*/,
                                                          std::span<const std::byte> /*pixels*/) {
    notImplemented("createTexture");
}

void VulkanRenderer::destroy(engine::graphics::TextureId /*texture*/) { notImplemented("destroy"); }

void VulkanRenderer::beginFrame(const engine::graphics::Color& /*clearColor*/) { notImplemented("beginFrame"); }

void VulkanRenderer::endFrame() { notImplemented("endFrame"); }

void VulkanRenderer::setCamera(const engine::graphics::Camera& /*camera*/) { notImplemented("setCamera"); }

void VulkanRenderer::draw(const engine::graphics::Sprite& /*sprite*/) { notImplemented("draw(Sprite)"); }

void VulkanRenderer::draw(const engine::graphics::RectShape& /*rect*/) { notImplemented("draw(RectShape)"); }

void VulkanRenderer::notImplemented(std::string_view function) {
    throw engine::exceptions::UnsupportedFeatureException("VulkanRenderer::" + std::string(function) +
                                                          " is not implemented yet");
}

}  // namespace rtype::vulkan
