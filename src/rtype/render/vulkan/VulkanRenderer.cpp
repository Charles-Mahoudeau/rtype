/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** VulkanRenderer
*/

#include "VulkanRenderer.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <glm/ext/vector_float2.hpp>
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
#include "descriptor/DescriptorAllocator.hpp"
#include "descriptor/DescriptorSetLayout.hpp"
#include "descriptor/DescriptorWriter.hpp"
#include "frame/FrameData.hpp"
#include "interop/vulkan/IVulkanSurfaceSource.hpp"
#include "memory/Image.hpp"
#include "plugins/backend/exceptions/PlatformExceptions.hpp"
#include "plugins/backend/graphics/Camera.hpp"
#include "plugins/backend/graphics/CameraView.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/RectShape.hpp"
#include "plugins/backend/graphics/Sprite.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/platform/IPlatform.hpp"
#include "plugins/backend/system/ExecutablePath.hpp"
#include "rendering/DrawConstants.hpp"
#include "rendering/DynamicRendering.hpp"
#include "rendering/FrameUniforms.hpp"
#include "rendering/ShaderLayout.hpp"
#include "rendering/SpriteBatch.hpp"
#include "rendering/SpritePass.hpp"
#include "texture/SamplerCache.hpp"
#include "texture/Texture.hpp"

namespace rtype::render::vulkan {

namespace {

/// @brief Waits on fences and acquires images without a timeout.
constexpr std::uint64_t kNoTimeout = std::numeric_limits<std::uint64_t>::max();

/// @brief Format of every texture: the RGBA8 pixels IRenderer::createTexture() takes, decoded from sRGB when sampled.
constexpr vk::Format kTextureFormat = vk::Format::eR8G8B8A8Srgb;

/// @brief Texture sets per descriptor pool: a new pool is added each time this many textures are alive.
constexpr std::uint32_t kTextureSetsPerPool = 64;

/// @return @p color (x, y, z, w = red, green, blue, alpha) as a Vulkan clear value: linear, an sRGB swapchain encodes
/// it.
vk::ClearColorValue toClearColor(const plugins::backend::graphics::Color& color) {
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

void VulkanRenderer::setup(plugins::backend::platform::IPlatform& platform) {
    surfaceSourceOf(platform).initLoader(core::Instance::getLoaderEntryPoint());
}

void VulkanRenderer::init(plugins::backend::platform::IPlatform& platform) {
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
    _uploader = std::make_unique<memory::Uploader>(*_device, *_allocator);
    const std::filesystem::path& shaderDirectory = _config.shaderDirectory;
    _shaders = std::make_unique<pipeline::ShaderCache>(
        *_device, shaderDirectory.is_absolute() ? shaderDirectory
                                                : plugins::backend::system::getExecutableDirectory() / shaderDirectory);
    _pipelineCache = std::make_unique<pipeline::PipelineCache>(*_device);
    _globals = std::make_unique<frame::FrameGlobals>(*_device, *_allocator, kFramesInFlight);
    _samplers = std::make_unique<texture::SamplerCache>(*_device, *_physicalDevice);
    _textureSetLayout = std::make_unique<descriptor::DescriptorSetLayout>(
        descriptor::DescriptorSetLayout::Builder{}
            .addBinding(rendering::kTextureBinding, vk::DescriptorType::eCombinedImageSampler,
                        vk::ShaderStageFlagBits::eFragment)
            .build(*_device));
    const std::array textureSizes{
        vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, kTextureSetsPerPool}};
    _textureDescriptors =
        std::make_unique<descriptor::DescriptorAllocator>(*_device, kTextureSetsPerPool, textureSizes);
    const std::array setLayouts{*_globals->getSetLayout().getLayout(), *_textureSetLayout->getLayout()};
    const std::array pushConstants{vk::PushConstantRange{
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(rendering::DrawConstants)}};
    _layout = std::make_unique<pipeline::PipelineLayout>(*_device, setLayouts, pushConstants);
    _spritePass =
        std::make_unique<rendering::SpritePass>(*_device, *_allocator, *_shaders, *_layout, _swapchain->getFormat(),
                                                &_pipelineCache->getCache(), kFramesInFlight);
    _startTime = std::chrono::steady_clock::now();
    _lastFrameTime = _startTime;
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

void VulkanRenderer::updateFrameUniforms(const plugins::backend::graphics::CameraView& view) {
    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    const vk::Extent2D extent = _swapchain->getExtent();
    _globals->update(
        _frames->getFrameIndex(),
        rendering::FrameUniforms{
            .time = std::chrono::duration<float>{now - _startTime}.count(),
            .deltaTime = std::chrono::duration<float>{now - _lastFrameTime}.count(),
            .framebufferSize = glm::vec2{static_cast<float>(extent.width), static_cast<float>(extent.height)},
            .viewProjection = view.getViewProjection(),
        });
    _lastFrameTime = now;
}

std::size_t VulkanRenderer::getLastRecordedFrame() const {
    const std::size_t frameIndex = _frames->getFrameIndex();
    return _acquiredImage ? frameIndex : (frameIndex + kFramesInFlight - 1) % kFramesInFlight;
}

plugins::backend::graphics::TextureId VulkanRenderer::createTexture(const plugins::backend::graphics::TextureDesc& desc,
                                                                    std::span<const std::byte> pixels) {
    if (!_uploader) {
        throw std::logic_error("VulkanRenderer::createTexture() called before init()");
    }
    const std::uint32_t maxSize = _physicalDevice->getLimits().maxImageDimension2D;
    if (desc.size.x == 0 || desc.size.y == 0 || desc.size.x > maxSize || desc.size.y > maxSize) {
        throw std::invalid_argument(
            std::format("VulkanRenderer::createTexture(): a {}x{} texture, sides must be 1 to {}", desc.size.x,
                        desc.size.y, maxSize));
    }
    memory::Image image{*_device, *_allocator, vk::Extent2D{desc.size.x, desc.size.y}, kTextureFormat,
                        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst};
    _uploader->upload(pixels, image);
    _uploader->flush();
    vk::raii::DescriptorSet set = _textureDescriptors->allocate(*_textureSetLayout);
    descriptor::DescriptorWriter{}
        .writeImage(rendering::kTextureBinding, *image.getImageView(), *_samplers->get(desc.filter, desc.repeat),
                    vk::ImageLayout::eShaderReadOnlyOptimal)
        .update(*_device, *set);
    return _textures.insert(texture::Texture{.image = std::move(image), .desc = desc, .set = std::move(set)});
}

void VulkanRenderer::destroy(plugins::backend::graphics::TextureId texture) {
    std::optional<texture::Texture> removed = _textures.erase(texture);
    if (removed) {
        _deletionQueue.defer(getLastRecordedFrame(), std::move(*removed));
    }
}

void VulkanRenderer::beginFrame(const plugins::backend::graphics::Color& clearColor) {
    if (!_frames) {
        throw std::logic_error("VulkanRenderer::beginFrame() called before init()");
    }
    if (_acquiredImage) {
        throw std::logic_error("VulkanRenderer::beginFrame() called twice without endFrame()");
    }
    _camera.reset();
    _sprites.clear();
    if (_framebufferSize.x == 0 || _framebufferSize.y == 0) {
        return;
    }
    _frames->getCurrentFrame().getInFlight().wait();
    _deletionQueue.flush(_frames->getFrameIndex());
    if (_swapchainOutOfDate) {
        recreateSwapchain();
    }
    const frame::FrameData& frame = _frames->getCurrentFrame();

    std::uint32_t imageIndex = 0;
    try {
        const vk::ResultValue<std::uint32_t> acquired =
            _swapchain->getSwapchain().acquireNextImage(kNoTimeout, *frame.getImageAvailable().getSemaphore());
        imageIndex = acquired.value;
        _swapchainOutOfDate = _swapchainOutOfDate || acquired.result == vk::Result::eSuboptimalKHR;
    } catch (const vk::OutOfDateKHRError&) {
        recreateSwapchain();
        return;
    }
    frame.getInFlight().reset();
    frame.getCommandPool().reset();

    frame.getCommandBuffer().begin();
    const vk::raii::CommandBuffer& commandBuffer = frame.getCommandBuffer().getCommandBuffer();
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
    const vk::raii::CommandBuffer& commandBuffer = frame.getCommandBuffer().getCommandBuffer();
    const std::size_t frameIndex = _frames->getFrameIndex();

    const vk::Extent2D extent = _swapchain->getExtent();
    const glm::uvec2 framebufferSize{extent.width, extent.height};
    const glm::vec2 viewSize{_config.virtualResolution};
    const plugins::backend::graphics::CameraView view{
        _camera.value_or(plugins::backend::graphics::CameraView::makeDefaultCamera(viewSize, framebufferSize)),
        viewSize, framebufferSize};
    updateFrameUniforms(view);
    _globals->bind(commandBuffer, *_layout, frameIndex);
    _spritePass->record(commandBuffer, frameIndex, _sprites, view.getViewport(),
                        [this](plugins::backend::graphics::TextureId texture) {
                            const texture::Texture* found = _textures.find(texture);
                            return found != nullptr ? *found->set : vk::DescriptorSet{};
                        });
    _sprites.clear();

    commandBuffer.endRendering();
    rendering::transitionImage(commandBuffer, _swapchain->getImages().at(imageIndex),
                               rendering::kColorAttachmentToPresent);
    frame.getCommandBuffer().end();

    const vk::Semaphore renderFinished = *_frames->getRenderFinished(imageIndex).getSemaphore();
    const vk::SemaphoreSubmitInfo waitInfo{*frame.getImageAvailable().getSemaphore(), 0,
                                           vk::PipelineStageFlagBits2::eColorAttachmentOutput};
    const vk::CommandBufferSubmitInfo commandBufferInfo{*commandBuffer};
    const vk::SemaphoreSubmitInfo signalInfo{renderFinished, 0, vk::PipelineStageFlagBits2::eAllCommands};
    _device->getGraphicsQueue().handle.submit2(vk::SubmitInfo2{}
                                                   .setWaitSemaphoreInfos(waitInfo)
                                                   .setCommandBufferInfos(commandBufferInfo)
                                                   .setSignalSemaphoreInfos(signalInfo),
                                               *frame.getInFlight().getFence());

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

void VulkanRenderer::setCamera(const plugins::backend::graphics::Camera& camera) { _camera = camera; }

void VulkanRenderer::draw(const plugins::backend::graphics::Sprite& sprite) {
    if (!_acquiredImage) {
        return;
    }
    const texture::Texture* texture = _textures.find(sprite.texture);
    if (texture == nullptr) {
        throw std::invalid_argument("VulkanRenderer::draw(): the sprite's texture was destroyed or never created");
    }
    _sprites.add(sprite.texture, rendering::SpriteInstance::fromSprite(sprite, texture->desc.size));
}

void VulkanRenderer::draw(const plugins::backend::graphics::RectShape& /*rect*/) { notImplemented("draw(RectShape)"); }

interop::vulkan::IVulkanSurfaceSource& VulkanRenderer::surfaceSourceOf(
    plugins::backend::platform::IPlatform& platform) {
    auto* surfaceSource = dynamic_cast<interop::vulkan::IVulkanSurfaceSource*>(&platform);
    if (surfaceSource == nullptr) {
        throw plugins::backend::exceptions::UnsupportedFeatureException(
            "VulkanRenderer needs a platform that implements IVulkanSurfaceSource (glfw, sdl...)");
    }
    return *surfaceSource;
}

void VulkanRenderer::notImplemented(std::string_view function) {
    throw plugins::backend::exceptions::UnsupportedFeatureException(
        std::format("VulkanRenderer::{} is not implemented yet", function));
}

}  // namespace rtype::render::vulkan
