/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** VulkanRenderer
*/

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "Export.hpp"
#include "core/DebugMessenger.hpp"
#include "core/DeletionQueue.hpp"
#include "core/Device.hpp"
#include "core/Instance.hpp"
#include "core/PhysicalDevice.hpp"
#include "core/SlotMap.hpp"
#include "frame/FrameGlobals.hpp"
#include "frame/FrameResources.hpp"
#include "interop/vulkan/IVulkanSurfaceSource.hpp"
#include "memory/Allocator.hpp"
#include "memory/Uploader.hpp"
#include "pipeline/PipelineCache.hpp"
#include "pipeline/ShaderCache.hpp"
#include "plugins/backend/graphics/Camera.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/IRenderer.hpp"
#include "plugins/backend/graphics/RectShape.hpp"
#include "plugins/backend/graphics/Sprite.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/platform/IPlatform.hpp"
#include "presentation/Swapchain.hpp"
#include "texture/Texture.hpp"

namespace rtype::render::vulkan {

/// @brief The Vulkan implementation of IRenderer.
///
/// @details init() creates the instance, the debug messenger (if requested), the surface, the device, the swapchain,
/// the frames in flight and the allocator. beginFrame() clears the next swapchain image with dynamic rendering and
/// endFrame() presents it. createTexture() uploads RGBA8 sRGB images. Drawing functions throw
/// UnsupportedFeatureException until they are implemented.
///
/// Needs a platform that implements interop::vulkan::IVulkanSurfaceSource (GlfwPlatform does).
class RTYPE_RENDER_VULKAN_API VulkanRenderer final : public plugins::backend::graphics::IRenderer {
  public:
    /// @brief Settings of the Vulkan backend, given at construction. Set only what differs from the defaults:
    /// @code
    /// VulkanRenderer renderer(VulkanRenderer::Config{.layers = {"VK_LAYER_KHRONOS_validation"}, .debugging =
    /// true});
    /// @endcode
    struct Config {
        std::string engineName = "R-Type Engine";       ///< Reported to the driver, with the window title as app name.
        std::uint32_t apiVersion = VK_API_VERSION_1_3;  ///< Highest Vulkan version the renderer uses.
        std::vector<std::string> layers;           ///< Instance layers to enable, e.g. VK_LAYER_KHRONOS_validation.
        std::vector<std::string> extraExtensions;  ///< Enabled on top of the ones the window needs.
        bool debugging = false;                    ///< Whether to create the DebugMessenger
        vk::DebugUtilsMessageSeverityFlagBitsEXT minSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;  ///< Minimum severity of messages the DebugMessenger
                                                                 ///< prints.
        vk::PhysicalDeviceType preferredDeviceType =
            vk::PhysicalDeviceType::eDiscreteGpu;  ///< Type of GPU favored when several suitable ones are found.
        bool synchronizationValidation = true;     ///< Synchronization validation (missing barriers, hazards), when
                                                   ///< VK_LAYER_KHRONOS_validation is in layers. Slow.
        bool bestPractices = false;  ///< Best practices warnings, when VK_LAYER_KHRONOS_validation is in layers.
        vk::PresentModeKHR presentMode =
            vk::PresentModeKHR::eFifo;  ///< Swapchain present mode: FIFO (vsync, always supported), MAILBOX (vsync,
                                        ///< a newer image replaces the pending one instead of waiting) or IMMEDIATE
                                        ///< (no vsync, may tear; benchmarks); FIFO when unsupported.
        std::filesystem::path shaderDirectory = "shaders";  ///< Where the game's SPIR-V shaders are loaded from.
                                                            ///< Relative: to the executable's directory, never to
                                                            ///< the working directory. Built-in shaders are embedded.
    };

    /// @brief Called after each swapchain recreation with the new swapchain: recreate there what depends on its size
    /// or format (depth, MSAA, HDR targets...).
    using SwapchainListener = std::function<void(const presentation::Swapchain&)>;

    explicit VulkanRenderer(Config config);
    /// @brief A renderer with the default Config.
    VulkanRenderer();
    ~VulkanRenderer() override;
    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;
    VulkanRenderer(VulkanRenderer&&) = delete;
    VulkanRenderer& operator=(VulkanRenderer&&) = delete;

    /// @brief Finds the platform's IVulkanSurfaceSource, and makes it use this library's Vulkan loader.
    /// @throws exceptions::UnsupportedFeatureException If the platform does not implement IVulkanSurfaceSource.
    void setup(plugins::backend::platform::IPlatform& platform) override;

    /// @brief Creates the instance (with the platform's extensions), the debug messenger and the window surface.
    /// @throws exceptions::UnsupportedFeatureException If the platform does not implement IVulkanSurfaceSource.
    /// @throws std::runtime_error If a layer or an extension is missing, or init() was already called.
    void init(plugins::backend::platform::IPlatform& platform) override;

    /// @brief Records the new framebuffer size; the next beginFrame() recreates the swapchain (or skips frames while it
    /// is 0x0, i.e. minimized).
    void resize(glm::uvec2 framebufferSize) override;

    /// @brief Registers @p listener, called after every swapchain recreation.
    void addSwapchainListener(SwapchainListener listener);

    /// @brief Waits for the current frame in flight, acquires the next swapchain image and starts rendering into it,
    /// cleared to @p clearColor.
    /// @details Recreates the swapchain first when it is out of date (resize, VK_ERROR_OUT_OF_DATE_KHR,
    /// VK_SUBOPTIMAL_KHR). The frame is skipped, and endFrame() then does nothing, while the window is minimized or
    /// when the acquire reports the swapchain out of date.
    /// @throws std::logic_error If init() was not called, or the previous frame was not ended.
    void beginFrame(const plugins::backend::graphics::Color& clearColor) override;

    /// @brief Ends rendering, submits the frame and presents the image. Does nothing if beginFrame() skipped it.
    void endFrame() override;

    /// @brief Creates a sampled R8G8B8A8_SRGB image of @p desc's size, and uploads @p pixels into it before returning
    /// (one round trip to the GPU).
    /// @throws std::logic_error If init() was not called.
    /// @throws std::invalid_argument If a side of desc.size is 0 or above the GPU's limit, or @p pixels is not
    /// desc.size.x * desc.size.y * 4 bytes.
    /// @throws vk::SystemError If the image or its memory cannot be created, or the upload fails.
    [[nodiscard]] plugins::backend::graphics::TextureId createTexture(
        const plugins::backend::graphics::TextureDesc& desc, std::span<const std::byte> pixels) override;

    /// @brief Forgets @p texture at once, and destroys its image once the frames that may draw it are done. Does
    /// nothing if @p texture was already destroyed or never created.
    void destroy(plugins::backend::graphics::TextureId texture) override;

    /// @name Not implemented yet: throw UnsupportedFeatureException
    /// @{
    void setCamera(const plugins::backend::graphics::Camera& camera) override;
    void draw(const plugins::backend::graphics::Sprite& sprite) override;
    void draw(const plugins::backend::graphics::RectShape& rect) override;
    /// @}

  private:
    /// @throws exceptions::UnsupportedFeatureException Always, naming the function.
    /// @return The platform's IVulkanSurfaceSource.
    /// @throws exceptions::UnsupportedFeatureException If the platform does not implement it.
    static interop::vulkan::IVulkanSurfaceSource& surfaceSourceOf(plugins::backend::platform::IPlatform& platform);

    [[noreturn]] static void notImplemented(std::string_view function);

    /// @brief Waits for the GPU, builds a new swapchain from the old one (oldSwapchain) and new renderFinished
    /// semaphores, defers the old ones to the deletion queue, then notifies the listeners. Does nothing but keep the
    /// swapchain out of date while the window is minimized.
    void recreateSwapchain();

    /// @brief Writes this frame's FrameUniforms (time, delta time, swapchain extent) into its uniform buffer.
    /// @warning Only after the frame's fence was waited for: the GPU may still read the buffer before.
    void updateFrameUniforms();

    /// @return The frame in flight whose fence covers every use recorded so far: the current one while a frame is
    /// recorded, the previous one between frames. What the GPU may still use is deferred to it.
    [[nodiscard]] std::size_t getLastRecordedFrame() const;

    /// @brief Frames the CPU may record while the GPU still works on previous ones.
    static constexpr std::size_t kFramesInFlight = 2;

    // Teardown order. Members are destroyed in reverse declaration order, after ~VulkanRenderer() has waited for the
    // GPU (vkDeviceWaitIdle). Each group below may only depend on the groups declared before it, so every Vulkan
    // object is destroyed before what it was created from. New members go in their group's slot, never at the end:
    //   1. Settings and state, no Vulkan object.
    //   2. Instance, debug messenger, surface, physical device, device (and its queues).
    //   3. Swapchain and its image views.
    //   4. Sync objects (fences, semaphores) and command pools / buffers.
    //   5. Allocator, then buffers, images and samplers (they must die before it).
    //   6. Pipelines, pipeline layouts, descriptor pools / sets, shader modules.   (shader modules only for now)
    //   7. Deletion queue: last declared, so it is flushed first, while everything it may hold is still alive.
    // The window outlives the renderer: Backend declares its platform before its renderer.

    // 1. Settings and state.
    Config _config;                               ///< Settings given at construction.
    glm::uvec2 _framebufferSize{0};               ///< Size of the window's framebuffer, in pixels.
    std::optional<std::uint32_t> _acquiredImage;  ///< Swapchain image of the frame being recorded, between
                                                  ///< beginFrame() and endFrame(); empty when the frame was skipped.
    bool _swapchainOutOfDate = false;  ///< Set on resize, VK_ERROR_OUT_OF_DATE_KHR or VK_SUBOPTIMAL_KHR; the next
                                       ///< beginFrame() recreates the swapchain and clears it.
    std::vector<SwapchainListener> _swapchainListeners;    ///< Called after each swapchain recreation.
    std::chrono::steady_clock::time_point _startTime;      ///< When init() ran: origin of FrameUniforms::time.
    std::chrono::steady_clock::time_point _lastFrameTime;  ///< When the previous frame began: for deltaTime.

    // 2. Context: created by init(), in this order.
    std::optional<core::Instance> _instance;                ///< The VkInstance.
    std::optional<core::DebugMessenger> _debugMessenger;    ///< Created by init() when debugging is enabled.
    vk::raii::SurfaceKHR _surface = nullptr;                ///< The window's surface.
    std::unique_ptr<core::PhysicalDevice> _physicalDevice;  ///< The chosen GPU.
    std::unique_ptr<core::Device> _device;                  ///< The logical device and its queues.

    // 3. Presentation.
    std::unique_ptr<presentation::Swapchain> _swapchain;  ///< The window's swapchain and its image views.
    // 4. Frames in flight: command pools / buffers and sync objects.
    std::unique_ptr<frame::FrameResources> _frames;  ///< Per-frame command buffers and sync objects.

    // 5. Memory: the allocator first, then the resources allocated from it.
    std::unique_ptr<memory::Allocator> _allocator;  ///< VMA allocator, destroyed after every buffer and image.
    std::unique_ptr<memory::Uploader> _uploader;    ///< Fills device local buffers and images through staging buffers.
    core::SlotMap<plugins::backend::graphics::TextureId, texture::Texture> _textures;  ///< By TextureId.

    // 6. Pipelines, descriptors and shaders.
    std::unique_ptr<pipeline::ShaderCache> _shaders;          ///< Shader modules, built-in and from the game.
    std::unique_ptr<pipeline::PipelineCache> _pipelineCache;  ///< Shared by every pipeline the renderer builds.
    std::unique_ptr<frame::FrameGlobals> _globals;            ///< Per-frame uniforms, at set 0, binding 0.

    // 7. Deferred deletion.
    core::DeletionQueue _deletionQueue{kFramesInFlight};  ///< Resources the GPU may still use, one bucket per frame.
};
}  // namespace rtype::render::vulkan
