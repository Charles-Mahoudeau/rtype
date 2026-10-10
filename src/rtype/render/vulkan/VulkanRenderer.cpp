/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** VulkanRenderer
*/

#include "VulkanRenderer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <glm/ext/vector_uint2.hpp>
#include <iostream>
#include <limits>
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
#include "frame/FrameData.hpp"
#include "interop/vulkan/IVulkanSurfaceSource.hpp"
#include "rendering/DynamicRendering.hpp"

namespace rtype::render::vulkan {

namespace {

/// @brief Waits on fences and acquires images without a timeout.
constexpr std::uint64_t kNoTimeout = std::numeric_limits<std::uint64_t>::max();

/// @return @p color (x, y, z, w = red, green, blue, alpha) as a Vulkan clear value: linear, an sRGB swapchain encodes
/// it.
vk::ClearColorValue toClearColor(const engine::graphics::Color& color) {
    return vk::ClearColorValue{std::array{color.x, color.y, color.z, color.w}};
}

}  // namespace

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
    _frames = std::make_unique<frame::FrameResources>(*_device, kFramesInFlight, _swapchain->getImages().size());

    _allocator = std::make_unique<memory::Allocator>(*_instance, *_physicalDevice, *_device);
}

void VulkanRenderer::resize(glm::uvec2 framebufferSize) {
    _framebufferSize = framebufferSize;
    _swapchainOutOfDate = true;
}

void VulkanRenderer::addSwapchainListener(SwapchainListener listener) {
    _swapchainListeners.push_back(std::move(listener));
}

void VulkanRenderer::recreateSwapchain() {
    if (_framebufferSize.x == 0 || _framebufferSize.y == 0) {
        _swapchainOutOfDate = true;
        return;
    }
    _device->getDevice().waitIdle();
    auto swapchain = std::make_unique<presentation::Swapchain>(*_physicalDevice, _surface, *_device, _framebufferSize,
                                                               _config.presentMode, *_swapchain->getSwapchain());
    const std::size_t frameIndex = _frames->getFrameIndex();
    _deletionQueue.defer(frameIndex, std::exchange(_swapchain, std::move(swapchain)));
    _deletionQueue.defer(frameIndex, _frames->recreateRenderFinished(*_device, _swapchain->getImages().size()));
    _swapchainOutOfDate = false;
    for (const SwapchainListener& listener : _swapchainListeners) {
        listener(*_swapchain);
    }
}

engine::graphics::TextureId VulkanRenderer::createTexture(const engine::graphics::TextureDesc& /*desc*/,
                                                          std::span<const std::byte> /*pixels*/) {
    notImplemented("createTexture");
}

void VulkanRenderer::destroy(engine::graphics::TextureId /*texture*/) { notImplemented("destroy"); }

void VulkanRenderer::beginFrame(const engine::graphics::Color& clearColor) {
    if (!_frames) {
        throw std::logic_error("VulkanRenderer::beginFrame() called before init()");
    }
    if (_acquiredImage) {
        throw std::logic_error("VulkanRenderer::beginFrame() called twice without endFrame()");
    }
    if (_framebufferSize.x == 0 || _framebufferSize.y == 0) {
        return;
    }
    const vk::raii::Device& device = _device->getDevice();
    if (device.waitForFences(*_frames->getCurrentFrame().getInFlight(), vk::True, kNoTimeout) != vk::Result::eSuccess) {
        throw std::runtime_error("VulkanRenderer: waiting for the frame's fence failed");
    }
    _deletionQueue.flush(_frames->getFrameIndex());
    if (_swapchainOutOfDate) {
        recreateSwapchain();
    }
    const frame::FrameData& frame = _frames->getCurrentFrame();

    std::uint32_t imageIndex = 0;
    try {
        const vk::ResultValue<std::uint32_t> acquired =
            _swapchain->getSwapchain().acquireNextImage(kNoTimeout, *frame.getImageAvailable());
        imageIndex = acquired.value;
        _swapchainOutOfDate = _swapchainOutOfDate || acquired.result == vk::Result::eSuboptimalKHR;
    } catch (const vk::OutOfDateKHRError&) {
        recreateSwapchain();
        return;
    }
    device.resetFences(*frame.getInFlight());
    frame.getCommandPool().reset();

    const vk::raii::CommandBuffer& commandBuffer = frame.getCommandBuffer();
    commandBuffer.begin(vk::CommandBufferBeginInfo{}.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    rendering::transitionImage(commandBuffer, _swapchain->getImages().at(imageIndex), rendering::kToColorAttachment);
    const vk::RenderingAttachmentInfo color =
        rendering::colorAttachment(*_swapchain->getImageViews().at(imageIndex), toClearColor(clearColor));
    commandBuffer.beginRendering(rendering::renderingInfo(_swapchain->getExtent(), {&color, 1}));
    _acquiredImage = imageIndex;
}

void VulkanRenderer::endFrame() {
    if (!_acquiredImage) {
        return;
    }
    const std::uint32_t imageIndex = *_acquiredImage;
    _acquiredImage.reset();
    const frame::FrameData& frame = _frames->getCurrentFrame();
    const vk::raii::CommandBuffer& commandBuffer = frame.getCommandBuffer();

    commandBuffer.endRendering();
    rendering::transitionImage(commandBuffer, _swapchain->getImages().at(imageIndex),
                               rendering::kColorAttachmentToPresent);
    commandBuffer.end();

    const vk::Semaphore renderFinished = *_frames->getRenderFinished(imageIndex);
    const vk::SemaphoreSubmitInfo waitInfo{*frame.getImageAvailable(), 0,
                                           vk::PipelineStageFlagBits2::eColorAttachmentOutput};
    const vk::CommandBufferSubmitInfo commandBufferInfo{*commandBuffer};
    const vk::SemaphoreSubmitInfo signalInfo{renderFinished, 0, vk::PipelineStageFlagBits2::eAllCommands};
    _device->getGraphicsQueue().handle.submit2(vk::SubmitInfo2{}
                                                   .setWaitSemaphoreInfos(waitInfo)
                                                   .setCommandBufferInfos(commandBufferInfo)
                                                   .setSignalSemaphoreInfos(signalInfo),
                                               *frame.getInFlight());

    const vk::SwapchainKHR swapchain = *_swapchain->getSwapchain();
    try {
        const vk::Result presented = _device->getPresentQueue().handle.presentKHR(vk::PresentInfoKHR{}
                                                                                      .setWaitSemaphores(renderFinished)
                                                                                      .setSwapchains(swapchain)
                                                                                      .setImageIndices(imageIndex));
        _swapchainOutOfDate = _swapchainOutOfDate || presented == vk::Result::eSuboptimalKHR;
    } catch (const vk::OutOfDateKHRError&) {
        _swapchainOutOfDate = true;
    }
    _frames->advance();
}

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
