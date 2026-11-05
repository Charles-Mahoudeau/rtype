/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** VulkanRenderer
*/

#include "VulkanRenderer.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
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
#include <system_error>
#include <utility>
#include <variant>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "command/CommandLabel.hpp"
#include "core/DebugMessenger.hpp"
#include "core/Instance.hpp"
#include "descriptor/DescriptorAllocator.hpp"
#include "descriptor/DescriptorSetLayout.hpp"
#include "descriptor/DescriptorWriter.hpp"
#include "exceptions/ShaderException.hpp"
#include "frame/FrameData.hpp"
#include "interop/vulkan/IVulkanSurfaceSource.hpp"
#include "material/GameMaterial.hpp"
#include "material/GameShader.hpp"
#include "memory/Image.hpp"
#include "particle/GpuParticleData.hpp"
#include "particle/GpuParticleSystem.hpp"
#include "particle/ParticleSimulationPass.hpp"
#include "pipeline/PipelineCacheFile.hpp"
#include "plugins/backend/exceptions/PlatformExceptions.hpp"
#include "plugins/backend/graphics/CameraView.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/Particle.hpp"
#include "plugins/backend/graphics/ParticleSystemDesc.hpp"
#include "plugins/backend/graphics/Shader.hpp"
#include "plugins/backend/graphics/Shape.hpp"
#include "plugins/backend/graphics/Sprite.hpp"
#include "plugins/backend/graphics/Text.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/graphics/View.hpp"
#include "plugins/backend/platform/IPlatform.hpp"
#include "plugins/backend/system/CacheDirectory.hpp"
#include "plugins/backend/system/ExecutablePath.hpp"
#include "rendering/DynamicRendering.hpp"
#include "rendering/FrameUniforms.hpp"
#include "rendering/MaterialBinding.hpp"
#include "rendering/ShaderLayout.hpp"
#include "rendering/ShapeBatch.hpp"
#include "rendering/ShapeInstance.hpp"
#include "rendering/ShapePass.hpp"
#include "rendering/SpriteBatch.hpp"
#include "rendering/SpritePass.hpp"
#include "texture/Premultiply.hpp"
#include "texture/SamplerCache.hpp"
#include "texture/Texture.hpp"

namespace rtype::render::vulkan {

namespace {

/// @brief Waits on fences and acquires images without a timeout.
constexpr std::uint64_t kNoTimeout = std::numeric_limits<std::uint64_t>::max();

/// @brief Format of the colour textures: the RGBA8 pixels IRenderer::createTexture() takes, decoded from sRGB when
/// sampled.
constexpr vk::Format kTextureFormat = vk::Format::eR8G8B8A8Srgb;

/// @brief Format of the data textures (TextureEncoding::kLinear): the same pixels, sampled as they are stored.
constexpr vk::Format kDataTextureFormat = vk::Format::eR8G8B8A8Unorm;

/// @brief Texture sets per descriptor pool: a new pool is added each time this many textures are alive.
constexpr std::uint32_t kTextureSetsPerPool = 64;
/// @brief Most GPU particle systems' sets in one descriptor pool; a new pool is added when it is full.
constexpr std::uint32_t kParticleSetsPerPool = 32;
/// @return What builds the pipeline of a game shader's @p target.
material::GameShader::PipelineBuilder pipelineBuilderOf(plugins::backend::graphics::ShaderTarget target) noexcept {
    switch (target) {
        case plugins::backend::graphics::ShaderTarget::kShape:
            return &rendering::ShapePass::createMaterialPipeline;
        case plugins::backend::graphics::ShaderTarget::kParticle:
            return &rendering::ParticlePass::createMaterialPipeline;
        case plugins::backend::graphics::ShaderTarget::kSprite:
        case plugins::backend::graphics::ShaderTarget::kGlyph:
            break;
    }
    return &rendering::SpritePass::createMaterialPipeline;
}

/// @brief Material sets (one per material and frame in flight) in one descriptor pool.
constexpr std::uint32_t kMaterialSetsPerPool = 64;

/// @return @p color (x, y, z, w = red, green, blue, alpha) as a Vulkan clear value: linear, an sRGB swapchain encodes
/// it.
/// @return Where the pipeline cache is kept for the Config::pipelineCache @p configured: relative to the user's cache
/// directory's rtype folder; empty if @p configured is empty or that directory is unknown.
std::filesystem::path resolvePipelineCachePath(const std::filesystem::path& configured) {
    if (configured.empty() || configured.is_absolute()) {
        return configured;
    }
    const std::filesystem::path cacheDirectory = plugins::backend::system::getCacheDirectory();
    return cacheDirectory.empty() ? std::filesystem::path{} : cacheDirectory / "rtype" / configured;
}

/// @return The pipeline cache saved at @p path by a previous launch on @p device, or nothing if there is none, or it is
/// invalid or comes from another GPU or driver (it would be rebuilt anyway, and some drivers crash on it).
std::vector<std::byte> readPipelineCache(const std::filesystem::path& path, const core::PhysicalDevice& device) {
    if (path.empty()) {
        return {};
    }
    std::vector<std::byte> data = pipeline::readPipelineCacheFile(path);
    if (std::error_code error; data.empty() && std::filesystem::exists(path, error)) {
        std::cerr << "[Vulkan] The pipeline cache '" << path.string()
                  << "' is damaged: the pipelines are compiled again\n";
        return {};
    }
    if (!data.empty() && !pipeline::isCompatiblePipelineCache(data, device.getPhysicalDevice().getProperties())) {
        std::cerr << "[Vulkan] The pipeline cache '" << path.string()
                  << "' is invalid or comes from another GPU or driver: the pipelines are compiled again\n";
        return {};
    }
    return data;
}

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
    if (_pipelineCache && !_pipelineCachePath.empty()) {
        try {
            pipeline::writePipelineCacheFile(_pipelineCachePath, _pipelineCache->getData());
        } catch (const std::exception& error) {
            std::cerr << "[Vulkan] The pipeline cache was not saved: " << error.what() << '\n';
        }
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
    _device = std::make_unique<core::Device>(*_physicalDevice, _config.debugging);
    _swapchain = std::make_unique<presentation::Swapchain>(*_physicalDevice, _surface, *_device, _framebufferSize,
                                                           _config.presentMode);
    _frames = std::make_unique<frame::FrameResources>(*_device, kFramesInFlight, _swapchain->getImages().size());

    _allocator = std::make_unique<memory::Allocator>(*_instance, *_physicalDevice, *_device);
    _uploader = std::make_unique<memory::Uploader>(*_device, *_allocator);
    const std::filesystem::path& shaderDirectory = _config.shaderDirectory;
    _shaders = std::make_unique<pipeline::ShaderCache>(
        *_device, shaderDirectory.is_absolute() ? shaderDirectory
                                                : plugins::backend::system::getExecutableDirectory() / shaderDirectory);
    _pipelineCachePath = resolvePipelineCachePath(_config.pipelineCache);
    _pipelineCache =
        std::make_unique<pipeline::PipelineCache>(*_device, readPipelineCache(_pipelineCachePath, *_physicalDevice));
    _globals = std::make_unique<frame::FrameGlobals>(*_device, *_allocator, kFramesInFlight);
    _device->setName(*_pipelineCache->getCache(), "Pipeline cache");
    _samplers = std::make_unique<texture::SamplerCache>(*_device, *_physicalDevice);
    _textureSetLayout = std::make_unique<descriptor::DescriptorSetLayout>(
        descriptor::DescriptorSetLayout::Builder{}
            .addBinding(rendering::kTextureBinding, vk::DescriptorType::eCombinedImageSampler,
                        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
            .build(*_device));
    const std::array textureSizes{
        vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, kTextureSetsPerPool}};
    _textureDescriptors =
        std::make_unique<descriptor::DescriptorAllocator>(*_device, kTextureSetsPerPool, textureSizes, "Texture sets");
    _device->setName(*_textureSetLayout->getLayout(), "Texture set layout");
    descriptor::DescriptorSetLayout::Builder particleSet;
    for (std::uint32_t binding = 0; binding < particle::kBindingCount; ++binding) {
        particleSet.addBinding(binding, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute);
    }
    _particleSetLayout = std::make_unique<descriptor::DescriptorSetLayout>(particleSet.build(*_device));
    const std::array particleSizes{
        vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, kParticleSetsPerPool * particle::kBindingCount}};
    _particleDescriptors = std::make_unique<descriptor::DescriptorAllocator>(*_device, kParticleSetsPerPool,
                                                                             particleSizes, "Particle sets");
    _device->setName(*_particleSetLayout->getLayout(), "Particle set layout");
    _particleSimulation = std::make_unique<particle::ParticleSimulationPass>(*_device, *_shaders, *_particleSetLayout,
                                                                             &_pipelineCache->getCache());
    const std::array setLayouts{*_globals->getSetLayout().getLayout(), *_textureSetLayout->getLayout()};
    const std::array pushConstants{vk::PushConstantRange{
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, rendering::kMaxPushConstantsSize}};
    _layout = std::make_unique<pipeline::PipelineLayout>(*_device, setLayouts, pushConstants);
    descriptor::DescriptorSetLayout::Builder materialSet;
    const vk::ShaderStageFlags materialStages = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
    materialSet.addBinding(material::kMaterialValuesBinding, vk::DescriptorType::eUniformBuffer, materialStages);
    for (std::uint32_t slot = 0; slot < plugins::backend::graphics::kShaderMaxTextures; ++slot) {
        materialSet.addBinding(material::kMaterialFirstTextureBinding + slot, vk::DescriptorType::eCombinedImageSampler,
                               materialStages);
    }
    _materialSetLayout = std::make_unique<descriptor::DescriptorSetLayout>(materialSet.build(*_device));
    const std::array materialSizes{
        vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, kMaterialSetsPerPool},
        vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler,
                               kMaterialSetsPerPool * plugins::backend::graphics::kShaderMaxTextures}};
    _materialDescriptors = std::make_unique<descriptor::DescriptorAllocator>(*_device, kMaterialSetsPerPool,
                                                                             materialSizes, "Material sets");
    _device->setName(*_materialSetLayout->getLayout(), "Material set layout");

    const std::array materialSetLayouts{*_globals->getSetLayout().getLayout(), *_textureSetLayout->getLayout(),
                                        *_materialSetLayout->getLayout()};
    _materialLayout = std::make_unique<pipeline::PipelineLayout>(*_device, materialSetLayouts, pushConstants);
    _device->setName(*_layout->getLayout(), "Shared pipeline layout");
    _device->setName(*_materialLayout->getLayout(), "Game shaders pipeline layout");
    createPasses();
    constexpr std::array<std::byte, 4> kWhite{std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}};
    _fallbackTexture = createTexture(plugins::backend::graphics::TextureDesc{.size = {1, 1}}, kWhite);
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
    const vk::Format oldFormat = _swapchain->getFormat();
    _deletionQueue.defer(frameIndex, std::exchange(_swapchain, std::move(swapchain)));
    _deletionQueue.defer(frameIndex, _frames->recreateRenderFinished(*_device, _swapchain->getImages().size()));
    if (_swapchain->getFormat() != oldFormat) {
        createPasses();
    }
    _swapchainOutOfDate = false;
    for (const SwapchainListener& listener : _swapchainListeners) {
        listener(*_swapchain);
    }
}

void VulkanRenderer::createPasses() {
    auto spritePass =
        std::make_unique<rendering::SpritePass>(*_device, *_allocator, *_shaders, *_layout, _swapchain->getFormat(),
                                                &_pipelineCache->getCache(), kFramesInFlight);
    auto particlePass =
        std::make_unique<rendering::ParticlePass>(*_device, *_allocator, *_shaders, *_layout, _swapchain->getFormat(),
                                                  &_pipelineCache->getCache(), kFramesInFlight);
    auto shapePass =
        std::make_unique<rendering::ShapePass>(*_device, *_allocator, *_shaders, *_layout, _swapchain->getFormat(),
                                               &_pipelineCache->getCache(), kFramesInFlight);
    const std::size_t frameIndex = _frames->getFrameIndex();
    if (_spritePass) {
        _deletionQueue.defer(frameIndex, std::exchange(_spritePass, std::move(spritePass)));
    } else {
        _spritePass = std::move(spritePass);
    }
    if (_particlePass) {
        _deletionQueue.defer(frameIndex, std::exchange(_particlePass, std::move(particlePass)));
    } else {
        _particlePass = std::move(particlePass);
    }
    if (_shapePass) {
        _deletionQueue.defer(frameIndex, std::exchange(_shapePass, std::move(shapePass)));
    } else {
        _shapePass = std::move(shapePass);
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
    const bool data = desc.encoding == plugins::backend::graphics::TextureEncoding::kLinear;
    memory::Image image{*_device, *_allocator, vk::Extent2D{desc.size.x, desc.size.y},
                        data ? kDataTextureFormat : kTextureFormat,
                        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst};
    if (data) {
        _uploader->upload(pixels, image);
    } else {
        // Premultiplied once here, so the shaders and the blending can assume it (rendering/Blend.hpp).
        _uploader->upload(texture::premultiplySrgb(pixels), image);
    }
    _uploader->flush();
    vk::raii::DescriptorSet set = _textureDescriptors->allocate(*_textureSetLayout);
    descriptor::DescriptorWriter{}
        .writeImage(rendering::kTextureBinding, *image.getImageView(), *_samplers->get(desc.filter, desc.repeat),
                    vk::ImageLayout::eShaderReadOnlyOptimal)
        .update(*_device, *set);
    const plugins::backend::graphics::TextureId id =
        _textures.insert(texture::Texture{.image = std::move(image), .desc = desc, .set = std::move(set)});
    const texture::Texture& created = *_textures.find(id);
    _device->setName(*created.image.getImage(), std::format("Texture {}", id.index));
    _device->setName(*created.image.getImageView(), std::format("Texture {} view", id.index));
    _device->setName(*created.set, std::format("Texture {} set", id.index));
    return id;
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
    _view.reset();
    _sprites.clear();
    _particleInstances.clear();
    _particleRanges.clear();
    _drawList.clear();
    _simulations.clear();
    _shapes.clear();
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
    _clearColor = clearColor;
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
        _view.value_or(plugins::backend::graphics::CameraView::makeDefaultView(viewSize, framebufferSize)), viewSize,
        framebufferSize};
    updateFrameUniforms(view);
    {
        const command::CommandLabel label{*_device, commandBuffer, "Particle simulation"};
        recordParticleSimulations(commandBuffer);
    }

    // Rendering starts here, not in beginFrame(): work that cannot be recorded inside a rendering scope (compute
    // dispatches) is recorded above, before it.
    rendering::transitionImage(commandBuffer, _swapchain->getImages().at(imageIndex), rendering::kToColorAttachment);
    const vk::RenderingAttachmentInfo color =
        rendering::colorAttachment(*_swapchain->getImageViews().at(imageIndex), toClearColor(_clearColor));
    commandBuffer.beginRendering(rendering::renderingInfo(_swapchain->getExtent(), {&color, 1}));
    _globals->bind(commandBuffer, *_layout, frameIndex);
    recordSpritesAndParticles(commandBuffer, frameIndex, view);
    const auto findShapeMaterial = [this, frameIndex](plugins::backend::graphics::MaterialId material,
                                                      plugins::backend::graphics::ShaderTarget target) {
        return findMaterial(material, target, frameIndex);
    };
    {
        const command::CommandLabel label{*_device, commandBuffer, "Shapes"};
        _shapePass->record(commandBuffer, frameIndex, _shapes, view.getViewport(), findTextureSet(_fallbackTexture),
                           findShapeMaterial);
    }
    _shapes.clear();

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

glm::vec2 VulkanRenderer::getViewSize() const { return glm::vec2{_config.virtualResolution}; }

void VulkanRenderer::setView(const plugins::backend::graphics::View& view) { _view = view; }

void VulkanRenderer::draw(const plugins::backend::graphics::Sprite& sprite) {
    if (!_acquiredImage) {
        return;
    }
    const texture::Texture* texture = _textures.find(sprite.texture);
    if (texture == nullptr) {
        throw std::invalid_argument("VulkanRenderer::draw(): the sprite's texture was destroyed or never created");
    }
    const rendering::SpriteInstance instance = rendering::SpriteInstance::fromSprite(sprite, texture->desc.size);
    if (sprite.material != plugins::backend::graphics::MaterialId{} && _materials.find(sprite.material) != nullptr) {
        _sprites.add(sprite.texture, sprite.material, instance, sprite.instanceValues);
        return;
    }
    _sprites.add(sprite.texture, instance);
}

void VulkanRenderer::draw(const plugins::backend::graphics::Shape& shape) {
    if (!_acquiredImage) {
        return;
    }
    const rendering::ShapeInstance instance = rendering::ShapeInstance::fromShape(shape);
    if (shape.material != plugins::backend::graphics::MaterialId{} && _materials.find(shape.material) != nullptr) {
        _shapes.add(shape.material, instance, shape.instanceValues);
        return;
    }
    _shapes.add(instance);
}

void VulkanRenderer::draw(const plugins::backend::graphics::Text& text) {
    if (!_acquiredImage) {
        return;
    }
    const texture::Texture* texture = _textures.find(text.texture);
    if (texture == nullptr) {
        throw std::invalid_argument("VulkanRenderer::draw(): the text's texture was destroyed or never created");
    }
    const plugins::backend::graphics::MaterialId material =
        _materials.find(text.material) != nullptr ? text.material : plugins::backend::graphics::MaterialId{};
    for (const plugins::backend::graphics::GlyphQuad& glyph : text.glyphs) {
        _sprites.addGlyph(text.texture, material, rendering::SpriteInstance::fromGlyph(text, glyph, texture->desc.size),
                          text.instanceValues);
    }
}

bool VulkanRenderer::supportsGpuParticles() const noexcept { return true; }

plugins::backend::graphics::ParticleSystemId VulkanRenderer::createParticleSystem(
    const plugins::backend::graphics::ParticleSystemDesc& desc) {
    if (!_uploader) {
        throw std::logic_error("VulkanRenderer::createParticleSystem() called before init()");
    }
    particle::GpuParticleSystem system{*_device, *_allocator, *_uploader, *_particleDescriptors, *_particleSetLayout,
                                       desc};
    _uploader->flush();
    return _particleSystems.insert(std::move(system));
}

void VulkanRenderer::destroy(plugins::backend::graphics::ParticleSystemId system) {
    std::optional<particle::GpuParticleSystem> removed = _particleSystems.erase(system);
    if (removed) {
        _deletionQueue.defer(getLastRecordedFrame(), std::move(*removed));
    }
}

void VulkanRenderer::simulate(plugins::backend::graphics::ParticleSystemId system,
                              const plugins::backend::graphics::ParticleEmission& emission) {
    if (_particleSystems.find(system) == nullptr) {
        throw std::invalid_argument("VulkanRenderer::simulate(): the particle system was destroyed or never created");
    }
    if (!_acquiredImage) {
        return;
    }
    const auto same = [system](const auto& simulation) { return simulation.first == system; };
    if (auto found = std::ranges::find_if(_simulations, same); found != _simulations.end()) {
        found->second = emission;
        return;
    }
    _simulations.emplace_back(system, emission);
}

void VulkanRenderer::recordParticleSimulations(const vk::raii::CommandBuffer& commandBuffer) {
    std::vector<particle::ParticleSimulationPass::Job> jobs;
    jobs.reserve(_simulations.size());
    for (const auto& [id, emission] : _simulations) {
        if (particle::GpuParticleSystem* system = _particleSystems.find(id)) {
            jobs.push_back(particle::ParticleSimulationPass::Job{.system = system, .emission = emission});
        }
    }
    _particleSimulation->record(commandBuffer, jobs);
    _simulations.clear();
}

void VulkanRenderer::draw(const plugins::backend::graphics::ParticleDraw& particles) {
    if (!_acquiredImage) {
        return;
    }
    const auto* system = std::get_if<plugins::backend::graphics::ParticleSystemId>(&particles.particles);
    if (system != nullptr && _particleSystems.find(*system) == nullptr) {
        throw std::invalid_argument("VulkanRenderer::draw(): the particle system was destroyed or never created");
    }
    const auto* instances =
        std::get_if<std::span<const plugins::backend::graphics::ParticleInstance>>(&particles.particles);
    if (instances != nullptr && instances->empty()) {
        return;
    }
    const texture::Texture* texture = _textures.find(particles.texture);
    if (texture == nullptr) {
        throw std::invalid_argument("VulkanRenderer::draw(): the particles' texture was destroyed or never created");
    }
    _drawList.addParticles(static_cast<std::uint32_t>(_sprites.getBatches().size()),
                           static_cast<std::uint32_t>(_particleRanges.size()));
    _sprites.split();
    ParticleRange range{.texture = particles.texture,
                        .constants = rendering::ParticleConstants::fromDraw(particles, texture->desc.size)};
    if (_materials.find(particles.material) != nullptr) {
        range.material = particles.material;
        range.instanceValues = particles.instanceValues;
    }
    if (system != nullptr) {
        range.system = *system;
    } else {
        range.firstInstance = static_cast<std::uint32_t>(_particleInstances.size());
        range.instanceCount = static_cast<std::uint32_t>(instances->size());
        _particleInstances.insert(_particleInstances.end(), instances->begin(), instances->end());
    }
    _particleRanges.push_back(range);
}

void VulkanRenderer::recordSpritesAndParticles(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex,
                                               const plugins::backend::graphics::CameraView& view) {
    _spritePass->upload(frameIndex, _sprites);
    _particlePass->upload(frameIndex, _particleInstances);
    const std::span<const rendering::SpriteBatch::Batch> batches = _sprites.getBatches();
    const auto findSet = [this](plugins::backend::graphics::TextureId texture) { return findTextureSet(texture); };
    const auto findSpriteMaterial = [this, frameIndex](plugins::backend::graphics::MaterialId material,
                                                       plugins::backend::graphics::ShaderTarget target) {
        return findMaterial(material, target, frameIndex);
    };
    for (const rendering::DrawList::Step& step : _drawList.getSteps(static_cast<std::uint32_t>(batches.size()))) {
        if (const auto* run = std::get_if<rendering::DrawList::SpriteRun>(&step)) {
            const command::CommandLabel label{*_device, commandBuffer, "Sprites"};
            _spritePass->record(commandBuffer, frameIndex, batches.subspan(run->firstBatch, run->batchCount),
                                view.getViewport(), findSet, findSpriteMaterial);
            continue;
        }
        const command::CommandLabel label{*_device, commandBuffer, "Particles"};
        const ParticleRange& range = _particleRanges.at(std::get<rendering::DrawList::ParticleStep>(step).draw);
        const rendering::MaterialBinding material =
            range.material != plugins::backend::graphics::MaterialId{}
                ? findMaterial(range.material, plugins::backend::graphics::ShaderTarget::kParticle, frameIndex)
                : rendering::MaterialBinding{};
        const rendering::ParticlePass::Draw draw{.constants = range.constants,
                                                 .textureSet = findTextureSet(range.texture),
                                                 .firstInstance = range.firstInstance,
                                                 .instanceCount = range.instanceCount,
                                                 .material = material,
                                                 .instanceValues = range.instanceValues};
        if (!range.system) {
            _particlePass->record(commandBuffer, frameIndex, draw, view.getViewport());
        } else if (const particle::GpuParticleSystem* system = _particleSystems.find(*range.system)) {
            _particlePass->recordIndirect(commandBuffer, draw, *system->getInstances().getBuffer(),
                                          *system->getIndirect().getBuffer(), view.getViewport());
        }
    }
    _sprites.clear();
    _particleInstances.clear();
    _particleRanges.clear();
    _drawList.clear();
}

vk::DescriptorSet VulkanRenderer::findTextureSet(plugins::backend::graphics::TextureId texture) const {
    const texture::Texture* found = _textures.find(texture);
    return found != nullptr ? *found->set : vk::DescriptorSet{};
}

bool VulkanRenderer::supportsShaders() const noexcept { return true; }

plugins::backend::graphics::ShaderId VulkanRenderer::createShader(
    const plugins::backend::graphics::CompiledShader& shader) {
    if (!_device) {
        throw std::logic_error("VulkanRenderer::createShader() called before init()");
    }
    try {
        return _gameShaders.insert(material::GameShader{*_device, shader});
    } catch (const exceptions::ShaderException& error) {
        throw std::invalid_argument(std::format("VulkanRenderer::createShader(): {}", error.what()));
    }
}

void VulkanRenderer::destroy(plugins::backend::graphics::ShaderId shader) {
    std::optional<material::GameShader> removed = _gameShaders.erase(shader);
    if (removed) {
        _deletionQueue.defer(getLastRecordedFrame(), std::move(*removed));
    }
}

plugins::backend::graphics::MaterialId VulkanRenderer::createMaterial(
    const plugins::backend::graphics::MaterialDesc& desc) {
    if (!_device) {
        throw std::logic_error("VulkanRenderer::createMaterial() called before init()");
    }
    const material::GameShader* shader = _gameShaders.find(desc.shader);
    if (shader == nullptr) {
        throw std::invalid_argument("VulkanRenderer::createMaterial(): the shader was destroyed or never created");
    }
    checkMaterial(desc, *shader, "createMaterial");
    return _materials.insert(material::GameMaterial{*_allocator, *_materialDescriptors, *_materialSetLayout,
                                                    shader->getMaterialSize(), desc, kFramesInFlight});
}

void VulkanRenderer::updateMaterial(plugins::backend::graphics::MaterialId material,
                                    const plugins::backend::graphics::MaterialDesc& desc) {
    material::GameMaterial* found = _materials.find(material);
    if (found == nullptr) {
        throw std::invalid_argument("VulkanRenderer::updateMaterial(): the material was destroyed or never created");
    }
    const material::GameShader* shader = _gameShaders.find(found->getShader());
    if (shader == nullptr) {
        throw std::invalid_argument("VulkanRenderer::updateMaterial(): the material's shader was destroyed");
    }
    checkMaterial(desc, *shader, "updateMaterial");
    found->update(desc);
}

void VulkanRenderer::destroy(plugins::backend::graphics::MaterialId material) {
    std::optional<material::GameMaterial> removed = _materials.erase(material);
    if (removed) {
        _deletionQueue.defer(getLastRecordedFrame(), std::move(*removed));
    }
}

void VulkanRenderer::checkMaterial(const plugins::backend::graphics::MaterialDesc& desc,
                                   const material::GameShader& shader, std::string_view function) const {
    if (desc.values.size() != shader.getMaterialSize()) {
        throw std::invalid_argument(
            std::format("VulkanRenderer::{}(): {} bytes of values for the {}-byte block of '{}'", function,
                        desc.values.size(), shader.getMaterialSize(), shader.getName()));
    }
    for (const plugins::backend::graphics::TextureId texture : desc.textures) {
        if (texture != plugins::backend::graphics::TextureId{} && _textures.find(texture) == nullptr) {
            throw std::invalid_argument(
                std::format("VulkanRenderer::{}(): a texture was destroyed or never created", function));
        }
    }
}

rendering::MaterialBinding VulkanRenderer::findMaterial(plugins::backend::graphics::MaterialId material,
                                                        plugins::backend::graphics::ShaderTarget target,
                                                        std::size_t frameIndex) {
    material::GameMaterial* found = _materials.find(material);
    material::GameShader* shader = found != nullptr ? _gameShaders.find(found->getShader()) : nullptr;
    if (shader == nullptr) {
        return {};
    }
    const pipeline::GraphicsPipeline* pipeline =
        shader->getPipeline(target, *_device, *_materialLayout, _swapchain->getFormat(), &_pipelineCache->getCache(),
                            pipelineBuilderOf(target));
    if (pipeline == nullptr) {
        return {};
    }
    const auto findTexture =
        [this](plugins::backend::graphics::TextureId texture) -> std::optional<material::GameMaterial::TextureBinding> {
        const texture::Texture* found = _textures.find(texture);
        if (found == nullptr) {
            return std::nullopt;
        }
        return material::GameMaterial::TextureBinding{
            .view = *found->image.getImageView(), .sampler = *_samplers->get(found->desc.filter, found->desc.repeat)};
    };
    return {.pipeline = pipeline,
            .set = found->prepare(*_device, frameIndex, findTexture, _fallbackTexture),
            .instanceValues = shader->hasInstanceValues()};
}

interop::vulkan::IVulkanSurfaceSource& VulkanRenderer::surfaceSourceOf(
    plugins::backend::platform::IPlatform& platform) {
    auto* surfaceSource = dynamic_cast<interop::vulkan::IVulkanSurfaceSource*>(&platform);
    if (surfaceSource == nullptr) {
        throw plugins::backend::exceptions::UnsupportedFeatureException(
            "VulkanRenderer needs a platform that implements IVulkanSurfaceSource (glfw, sdl...)");
    }
    return *surfaceSource;
}

}  // namespace rtype::render::vulkan
