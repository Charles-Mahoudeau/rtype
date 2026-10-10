/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** VulkanRenderer
*/

#include "VulkanRenderer.hpp"

#include <cstddef>
#include <format>
#include <glm/ext/vector_uint2.hpp>
#include <iostream>
#include <memory>
#include <optional>
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
#include "interop/vulkan/IVulkanSurfaceSource.hpp"

namespace rtype::render::vulkan {

VulkanRenderer::VulkanRenderer(Config config) : _config(std::move(config)) {}

VulkanRenderer::VulkanRenderer() : VulkanRenderer(Config{}) {}

VulkanRenderer::~VulkanRenderer() {
    if (!_device) {
        return;
    }
    try {
        _device->getDevice().waitIdle();
    } catch (const vk::SystemError& error) {
        std::cerr << "[Vulkan] vkDeviceWaitIdle failed at shutdown: " << error.what() << '\n';
    }
}

void VulkanRenderer::setup(engine::platform::IPlatform& platform) {
    surfaceSourceOf(platform).initLoader(core::Instance::getLoaderEntryPoint());
}

void VulkanRenderer::init(engine::platform::IPlatform& platform) {
    if (_instance) {
        throw std::runtime_error("VulkanRenderer::init() called twice");
    }

    interop::vulkan::IVulkanSurfaceSource& surfaceSource = surfaceSourceOf(platform);

    std::vector<const char*> extensions = surfaceSource.getRequiredExtensions();
    for (const std::string& extension : _config.extraExtensions) {
        extensions.push_back(extension.c_str());
    }
    if (_config.debugging) {
        extensions.push_back(core::DebugMessenger::kExtensionName);
    }
    std::vector<const char*> layers;
    layers.reserve(_config.layers.size());
    for (const std::string& layer : _config.layers) {
        layers.push_back(layer.c_str());
    }
    _instance.emplace(
        platform.getTitle(), _config.engineName, _config.apiVersion, std::move(extensions), std::move(layers),
        core::ValidationOptions{.synchronization = _config.synchronizationValidation,
                                .bestPractices = _config.bestPractices},
        _config.debugging ? std::optional{core::DebugMessenger::makeCreateInfo(_config.minSeverity)} : std::nullopt);
    if (_config.debugging) {
        _debugMessenger.emplace(*_instance, _config.minSeverity);
    }

    auto* const instance = static_cast<VkInstance>(*_instance->getInstance());
    _surface = vk::raii::SurfaceKHR(_instance->getInstance(), surfaceSource.createSurface(instance));
    _framebufferSize = platform.getFramebufferSize();
    _physicalDevice = std::make_unique<core::PhysicalDevice>(*_instance, _surface, _config.preferredDeviceType);
    _device = std::make_unique<core::Device>(*_physicalDevice);
    _swapchain = std::make_unique<presentation::Swapchain>(*_physicalDevice, _surface, *_device, _framebufferSize,
                                                           _config.presentMode);

    _allocator = std::make_unique<memory::Allocator>(*_instance, *_physicalDevice, *_device);
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

interop::vulkan::IVulkanSurfaceSource& VulkanRenderer::surfaceSourceOf(engine::platform::IPlatform& platform) {
    auto* surfaceSource = dynamic_cast<interop::vulkan::IVulkanSurfaceSource*>(&platform);
    if (surfaceSource == nullptr) {
        throw engine::exceptions::UnsupportedFeatureException(
            "VulkanRenderer needs a platform that implements IVulkanSurfaceSource (glfw, sdl...)");
    }
    return *surfaceSource;
}

void VulkanRenderer::notImplemented(std::string_view function) {
    throw engine::exceptions::UnsupportedFeatureException(
        std::format("VulkanRenderer::{} is not implemented yet", function));
}

}  // namespace rtype::render::vulkan
