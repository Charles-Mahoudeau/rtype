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
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "Export.hpp"
#include "core/DebugMessenger.hpp"
#include "core/DeletionQueue.hpp"
#include "core/Device.hpp"
#include "core/Instance.hpp"
#include "core/PhysicalDevice.hpp"
#include "core/SlotMap.hpp"
#include "descriptor/DescriptorAllocator.hpp"
#include "descriptor/DescriptorSetLayout.hpp"
#include "frame/FrameGlobals.hpp"
#include "frame/FrameResources.hpp"
#include "interop/vulkan/IVulkanSurfaceSource.hpp"
#include "material/GameMaterial.hpp"
#include "material/GameShader.hpp"
#include "memory/Allocator.hpp"
#include "memory/Uploader.hpp"
#include "particle/GpuParticleSystem.hpp"
#include "particle/ParticleSimulationPass.hpp"
#include "pipeline/PipelineCache.hpp"
#include "pipeline/PipelineLayout.hpp"
#include "pipeline/ShaderCache.hpp"
#include "presentation/Swapchain.hpp"
#include "rendering/DrawList.hpp"
#include "rendering/MaterialBinding.hpp"
#include "rendering/ParticleConstants.hpp"
#include "rendering/ParticlePass.hpp"
#include "rendering/ShapeBatch.hpp"
#include "rendering/ShapePass.hpp"
#include "rendering/SpriteBatch.hpp"
#include "rendering/SpritePass.hpp"
#include "rtype/contract/platform/IPlatform.hpp"
#include "rtype/contract/renderer/IRenderer.hpp"
#include "rtype/contract/renderer/drawing/Color.hpp"
#include "rtype/contract/renderer/drawing/Shape.hpp"
#include "rtype/contract/renderer/drawing/Sprite.hpp"
#include "rtype/contract/renderer/drawing/Text.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"
#include "rtype/contract/renderer/particle/Particle.hpp"
#include "rtype/contract/renderer/particle/ParticleSystemDesc.hpp"
#include "rtype/contract/renderer/shaders/CompiledShader.hpp"
#include "rtype/contract/renderer/view/CameraView.hpp"
#include "rtype/contract/renderer/view/View.hpp"
#include "texture/SamplerCache.hpp"
#include "texture/Texture.hpp"

namespace rtype::render::vulkan {

/// @brief The Vulkan implementation of IRenderer.
///
/// @details init() creates the instance, the debug messenger (if requested), the surface, the device, the swapchain,
/// the frames in flight and the allocator. beginFrame() acquires the next swapchain image and starts recording;
/// endFrame() records what must come before rendering (compute), clears the image with dynamic rendering, records the
/// draws and presents it. createTexture() uploads RGBA8 sRGB images, premultiplied by their alpha
/// (texture::premultiplySrgb()), and data (TextureEncoding::kLinear) as it is. draw(Sprite) and draw(Text) collect the
/// frame's sprites, and endFrame() draws them in batches (one instanced draw per run of sprites sharing a texture),
/// through the view given to setView(). draw(Shape) collects the frame's shapes, and endFrame() draws them after the
/// sprites, one instanced draw per run of shapes sharing a material, from their signed distance
/// (rendering/ShapePass.hpp). Sprites and shapes are blended with premultiplied alpha: every
/// contract::renderer::BlendMode is drawn by the same pipeline, read per instance (the shaders ABI's
/// fragment_common.glsl), so the mode never splits a batch. A sprite or a shape with a game material (createShader(),
/// createMaterial()) is drawn by its shader's pipeline, built the first time it is drawn, with the material's set 2
/// (material/GameMaterial.hpp); a run of sprites sharing a texture and a material stays one batch.
///
/// Needs a platform that implements interop::vulkan::IVulkanSurfaceSource (GlfwPlatform does).
class RTYPE_RENDER_VULKAN_API VulkanRenderer final : public contract::renderer::IRenderer {
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
        std::filesystem::path shaderDirectory = "shaders";  ///< Where the built-in SPIR-V shaders are loaded from.
                                                            ///< Relative: to the executable's directory, never to
                                                            ///< the working directory. Built-in shaders are embedded.
        std::filesystem::path pipelineCache = "pipeline_cache.bin";  ///< Where the compiled pipelines are kept between
                                                                     ///< launches: read by init(), written when the
                                                                     ///< renderer is destroyed. Relative: to the
                                                                     ///< user's cache directory's rtype folder
                                                                     ///< (getCacheDirectory()). Empty: not kept.
        glm::uvec2 virtualResolution{0};  ///< The resolution the game is designed for, e.g. 1920x1080: the camera shows
                                          ///< that many world units at zoom 1 whatever the window size, letterboxed.
                                          ///< 0x0: the window's framebuffer size, without bars.
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
    void setup(contract::platform::IPlatform& platform) override;

    /// @brief Creates the instance (with the platform's extensions), the debug messenger and the window surface.
    /// @throws exceptions::UnsupportedFeatureException If the platform does not implement IVulkanSurfaceSource.
    /// @throws std::runtime_error If a layer or an extension is missing, or init() was already called.
    void init(contract::platform::IPlatform& platform) override;

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
    void beginFrame(const contract::renderer::Color& clearColor) override;

    /// @brief Records the frame (rendering starts here: compute work is recorded before it), submits it and presents
    /// the image. Does nothing if beginFrame() skipped it.
    void endFrame() override;

    /// @brief Creates a sampled image of @p desc's size, R8G8B8A8_SRGB for colours or R8G8B8A8_UNORM for data
    /// (desc.encoding), uploads @p pixels into it before returning (one round trip to the GPU; colours premultiplied
    /// by their alpha, data as it is), and its descriptor set: the image with the sampler of desc's filter and repeat
    /// mode, at rendering::kTextureSet, rendering::kTextureBinding.
    /// @throws std::logic_error If init() was not called.
    /// @throws std::invalid_argument If a side of desc.size is 0 or above the GPU's limit, or @p pixels is not
    /// desc.size.x * desc.size.y * 4 bytes.
    /// @throws vk::SystemError If the image or its memory cannot be created, or the upload fails.
    [[nodiscard]] contract::renderer::TextureId createTexture(const contract::renderer::TextureDesc& desc,
                                                              std::span<const std::byte> pixels) override;

    /// @brief Forgets @p texture at once, and destroys its image once the frames that may draw it are done. Does
    /// nothing if @p texture was already destroyed or never created.
    void destroy(contract::renderer::TextureId texture) override;

    /// @return Config::virtualResolution, or 0x0 when the view is the framebuffer.
    [[nodiscard]] glm::vec2 getViewSize() const override;

    /// @brief Sets the view of the frame (contract::renderer::CameraView, with Config::virtualResolution): the last one
    /// set before endFrame() applies to every sprite of the frame. Until it is called, a frame shows the view from the
    /// world's origin at its top-left corner.
    void setView(const contract::renderer::View& view) override;

    /// @brief Adds @p sprite to the frame, drawn by endFrame() on top of the sprites drawn before it. Does nothing if
    /// beginFrame() skipped the frame.
    /// @throws std::invalid_argument If its texture was destroyed or never created.
    void draw(const contract::renderer::Sprite& sprite) override;

    /// @brief Adds @p shape to the frame, drawn by endFrame() after every sprite, on top of the shapes drawn before
    /// it, with its material (the default one if it no longer exists). Does nothing if beginFrame() skipped the frame.
    void draw(const contract::renderer::Shape& shape) override;

    /// @brief Adds the glyphs of @p text to the frame's sprites, one instance each, drawn by the glyph variant of the
    /// default shader (shaders/glyph.frag): batched with the glyphs before and after it that share its atlas, in the
    /// sprites' draw order. Does nothing if beginFrame() skipped the frame.
    /// @throws std::invalid_argument If its texture was destroyed or never created.
    void draw(const contract::renderer::Text& text) override;

    /// @brief Records @p items for this frame's overlay, drawn at endFrame() over everything else: see
    /// IRenderer::drawOverlay(). Each run of items of one kind and one clip is a scissored draw of its own.
    /// @throws std::invalid_argument If the texture of an item was destroyed or never created.
    void drawOverlay(std::span<const contract::renderer::OverlayItem> items) override;

    /// @return true: particle systems are simulated by compute shaders and drawn indirectly.
    [[nodiscard]] bool supportsGpuParticles() const noexcept override;

    /// @brief Creates a particle::GpuParticleSystem: its buffers, filled before this returns, and its descriptor set.
    /// @throws std::logic_error If init() was not called.
    /// @throws exceptions::ParticleException If @p desc is invalid.
    [[nodiscard]] contract::renderer::ParticleSystemId createParticleSystem(
        const contract::renderer::ParticleSystemDesc& desc) override;
    /// @brief Destroys a GPU particle system once the GPU is done with it; does nothing for one already destroyed.
    void destroy(contract::renderer::ParticleSystemId system) override;
    /// @brief Simulates @p system this frame with @p emission (particle::ParticleSimulationPass), recorded by
    /// endFrame() before rendering begins. Called twice for one system in a frame, the last emission wins. Does
    /// nothing but check @p system if beginFrame() skipped the frame.
    /// @throws std::invalid_argument If @p system was destroyed or never created.
    void simulate(contract::renderer::ParticleSystemId system,
                  const contract::renderer::ParticleEmission& emission) override;
    /// @brief Draws the instances of a CPU simulation, or the live particles of a GPU system (an indirect draw, as
    /// many as its last simulation left), among the sprites in call order (rendering/DrawList.hpp), with
    /// rendering::ParticlePass.
    /// @throws std::invalid_argument If the texture or the GPU system was destroyed or never created.
    void draw(const contract::renderer::ParticleDraw& particles) override;

    /// @return true: sprites and shapes are drawn with game shaders. Texts and particles are drawn with the default
    /// material for now.
    [[nodiscard]] bool supportsShaders() const noexcept override;

    /// @brief Creates the modules of @p shader's variants (material::GameShader); its pipelines are built the first
    /// time it is drawn.
    /// @throws std::logic_error If init() was not called.
    /// @throws std::invalid_argument If @p shader is invalid or a stage is not SPIR-V.
    [[nodiscard]] contract::renderer::ShaderId createShader(const contract::renderer::CompiledShader& shader) override;
    /// @brief Destroys a shader once the GPU is done with it; does nothing for one already destroyed. Its materials
    /// are then drawn as the default material.
    void destroy(contract::renderer::ShaderId shader) override;
    /// @brief Creates a material: per frame in flight, a uniform buffer for its values and its set 2. Unset textures
    /// are a white pixel.
    /// @throws std::logic_error If init() was not called.
    /// @throws std::invalid_argument If its shader or a texture was destroyed or never created, or the values are not
    /// the size of the shader's material block.
    [[nodiscard]] contract::renderer::MaterialId createMaterial(const contract::renderer::MaterialDesc& desc) override;
    /// @brief Changes a material's values and textures; each frame in flight takes them when it is next recorded.
    /// @throws std::invalid_argument As createMaterial(), or if @p material was destroyed or never created.
    void updateMaterial(contract::renderer::MaterialId material, const contract::renderer::MaterialDesc& desc) override;
    /// @brief Destroys a material once the GPU is done with it; does nothing for one already destroyed.
    void destroy(contract::renderer::MaterialId material) override;

  private:
    /// @brief Overlay draws that share a kind and a clip, recorded at endFrame().
    struct OverlayRun {
        bool shapes = false;                                 ///< Batches of _overlayShapes, or of _overlaySprites.
        std::uint32_t firstBatch = 0;                        ///< Index of its first batch.
        std::uint32_t batchCount = 0;                        ///< Number of batches.
        std::optional<contract::renderer::math::Rect> clip;  ///< What it stays in, in view pixels.
    };

    /// @brief A particle draw of the frame, recorded at endFrame().
    struct ParticleRange {
        contract::renderer::TextureId texture;   ///< Its set is found at endFrame(), as for the sprites.
        rendering::ParticleConstants constants;  ///< Uv rect, blend mode and alignment.
        std::uint32_t firstInstance = 0;         ///< First instance, in _particleInstances.
        std::uint32_t instanceCount = 0;         ///< Number of instances.
        std::optional<contract::renderer::ParticleSystemId> system;  ///< A GPU system, drawn indirectly.
        contract::renderer::MaterialId material;                     ///< Its material; the default one if 0.
        contract::renderer::ShaderInstanceValues instanceValues{};   ///< Of its material.
    };

    /// @return The platform's IVulkanSurfaceSource.
    /// @throws exceptions::UnsupportedFeatureException If the platform does not implement it.
    static interop::vulkan::IVulkanSurfaceSource& surfaceSourceOf(contract::platform::IPlatform& platform);

    /// @brief Waits for the GPU, builds a new swapchain from the old one (oldSwapchain) and new renderFinished
    /// semaphores, defers the old ones to the deletion queue, rebuilds the passes if the image format changed, then
    /// notifies the listeners. Does nothing but keep the swapchain out of date while the window is minimized.
    void recreateSwapchain();

    /// @brief Builds the sprite, particle and shape passes for the swapchain's image format, deferring the previous
    /// ones (their pipelines render into the old format) to the deletion queue.
    void createPasses();

    /// @brief Writes this frame's FrameUniforms (time, delta time, swapchain extent, @p view's matrix) into its uniform
    /// buffer.
    /// @warning Only after the frame's fence was waited for: the GPU may still read the buffer before.
    void updateFrameUniforms(const contract::renderer::CameraView& view);

    /// @brief Adds @p sprite to @p batch.
    /// @throws std::invalid_argument If its texture was destroyed or never created.
    void addTo(rendering::SpriteBatch& batch, const contract::renderer::Sprite& sprite);

    /// @brief Adds the glyphs of @p text to @p batch.
    /// @throws std::invalid_argument If its texture was destroyed or never created.
    void addTo(rendering::SpriteBatch& batch, const contract::renderer::Text& text);

    /// @brief Adds @p shape to @p batch.
    void addTo(rendering::ShapeBatch& batch, const contract::renderer::Shape& shape);

    /// @brief Records the overlay of the frame, in view pixels, inside dynamic rendering, then empties it.
    /// @param world The frame's view, for its viewport.
    void recordOverlay(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex,
                       const contract::renderer::CameraView& world, glm::vec2 viewSize, glm::uvec2 framebufferSize);

    /// @brief Records the simulation of the frame's GPU systems (_simulations), before rendering begins.
    void recordParticleSimulations(const vk::raii::CommandBuffer& commandBuffer);

    /// @brief Uploads the frame's sprites and particles, and records them in the order of _drawList, into @p view's
    /// viewport.
    void recordSpritesAndParticles(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex,
                                   const contract::renderer::CameraView& view);

    /// @return The descriptor set of @p texture, or a null handle if it no longer exists.
    [[nodiscard]] vk::DescriptorSet findTextureSet(contract::renderer::TextureId texture) const;

    /// @return What @p target is drawn with for @p material in frame @p frameIndex: its shader's
    /// pipeline, built if needed, and its set, brought up to date; a null pipeline if it or its shader no longer
    /// exists, or if its shader does not serve @p target.
    [[nodiscard]] rendering::MaterialBinding findMaterial(contract::renderer::MaterialId material,
                                                          contract::renderer::ShaderTarget target,
                                                          std::size_t frameIndex);

    /// @brief Checks that @p desc fits @p shader: values of its block's size, and textures that exist.
    /// @throws std::invalid_argument Naming @p function if not.
    void checkMaterial(const contract::renderer::MaterialDesc& desc, const material::GameShader& shader,
                       std::string_view function) const;

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
    //   6. Pipelines, pipeline layouts, descriptor pools / sets, shader modules. The textures come last: each holds
    //      an image (group 5) and a descriptor set from _textureDescriptors.
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
    std::chrono::steady_clock::time_point _lastFrameTime;  ///< When the previous frame ended: for deltaTime.
    std::optional<contract::renderer::View> _view;         ///< From setView(); reset by beginFrame().
    std::filesystem::path _pipelineCachePath;     ///< Config::pipelineCache resolved by init(); empty: not kept.
    contract::renderer::Color _clearColor{0.0F};  ///< From beginFrame(): what endFrame() clears with.
    rendering::SpriteBatch _sprites;              ///< The frame's sprites; cleared by beginFrame().
    rendering::ShapeBatch _shapes;                ///< The frame's shapes; cleared by beginFrame().
    rendering::SpriteBatch _overlaySprites;       ///< The overlay's sprites and glyphs; cleared by beginFrame().
    rendering::ShapeBatch _overlayShapes;         ///< The overlay's shapes; cleared by beginFrame().
    std::vector<OverlayRun> _overlayRuns;         ///< The overlay, in draw order; cleared by beginFrame().
    std::vector<contract::renderer::ParticleInstance> _particleInstances;  ///< The frame's CPU particles.
    std::vector<ParticleRange> _particleRanges;  ///< The frame's particle draws; cleared by beginFrame().
    rendering::DrawList _drawList;               ///< Where the particle draws sit among the sprite batches.
    std::vector<std::pair<contract::renderer::ParticleSystemId, contract::renderer::ParticleEmission>>
        _simulations;  ///< The GPU systems to simulate this frame, by id; cleared by beginFrame().
    contract::renderer::TextureId _fallbackTexture;  ///< A white pixel: a material's unset textures.

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
    std::unique_ptr<texture::SamplerCache> _samplers;  ///< One sampler per filter and repeat mode.

    // 6. Pipelines, descriptors and shaders.
    std::unique_ptr<pipeline::ShaderCache> _shaders;          ///< Shader modules, built-in and from the game.
    std::unique_ptr<pipeline::PipelineCache> _pipelineCache;  ///< Shared by every pipeline the renderer builds.
    std::unique_ptr<frame::FrameGlobals> _overlayGlobals;     ///< The frame uniforms of the overlay: its own view.
    std::unique_ptr<frame::FrameGlobals> _globals;            ///< Per-frame uniforms, at set 0, binding 0.
    std::unique_ptr<descriptor::DescriptorSetLayout> _textureSetLayout;     ///< Set 1: one combined image sampler.
    std::unique_ptr<descriptor::DescriptorAllocator> _textureDescriptors;   ///< Allocates the textures' sets.
    std::unique_ptr<descriptor::DescriptorSetLayout> _particleSetLayout;    ///< Set 0 of the particle compute shaders.
    std::unique_ptr<descriptor::DescriptorAllocator> _particleDescriptors;  ///< Allocates the GPU systems' sets.
    std::unique_ptr<descriptor::DescriptorSetLayout> _materialSetLayout;    ///< Set 2: a block and three textures.
    std::unique_ptr<descriptor::DescriptorAllocator> _materialDescriptors;  ///< Allocates the materials' sets.
    std::unique_ptr<pipeline::PipelineLayout> _layout;  ///< The shared layout (rendering/ShaderLayout.hpp) of every
                                                        ///< pipeline: sets 0 and 1, and the push constants.
    std::unique_ptr<pipeline::PipelineLayout> _materialLayout;  ///< The game shaders' layout: the shared one and set 2.
    std::unique_ptr<rendering::SpritePass> _spritePass;         ///< Draws _sprites at endFrame().
    std::unique_ptr<rendering::SpritePass> _overlaySpritePass;  ///< Draws _overlaySprites, with buffers of its own.
    std::unique_ptr<rendering::ShapePass> _overlayShapePass;    ///< Draws _overlayShapes, with buffers of its own.
    std::unique_ptr<rendering::ShapePass> _shapePass;           ///< Draws _shapes at endFrame(), after the sprites.
    std::unique_ptr<rendering::ParticlePass> _particlePass;  ///< Draws _particleRanges at endFrame(), among _sprites.
    std::unique_ptr<particle::ParticleSimulationPass> _particleSimulation;     ///< Runs _simulations at endFrame().
    core::SlotMap<contract::renderer::TextureId, texture::Texture> _textures;  ///< By TextureId.
    core::SlotMap<contract::renderer::ParticleSystemId, particle::GpuParticleSystem>
        _particleSystems;                                                              ///< By ParticleSystemId.
    core::SlotMap<contract::renderer::ShaderId, material::GameShader> _gameShaders;    ///< By ShaderId.
    core::SlotMap<contract::renderer::MaterialId, material::GameMaterial> _materials;  ///< By MaterialId.

    // 7. Deferred deletion.
    core::DeletionQueue _deletionQueue{kFramesInFlight};  ///< Resources the GPU may still use, one bucket per frame.
};
}  // namespace rtype::render::vulkan
