/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpritePass
*/

#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "SpriteBatch.hpp"
#include "plugins/backend/graphics/CameraView.hpp"
#include "plugins/backend/graphics/Shader.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"

namespace rtype::render::vulkan::rendering {

/// @brief Draws a frame's SpriteBatch: one instanced draw of a unit quad per batch, with premultiplied alpha (each
/// sprite's graphics::BlendMode is read per instance by the shaders, so it never splits a batch).
///
/// @details Owns the pipelines of the default effect (on the shared layout): sprites (shaders/sprite.vert and
/// sprite.frag) and glyphs (sprite.vert and glyph.frag). Per frame in flight, it owns an instance buffer and a buffer
/// of the game materials' instance values, CPU-written and read by the GPU as per-instance vertex attributes. A frame's
/// buffers are only used by that frame, whose fence was waited for before recording, so they are rewritten, or
/// replaced by bigger ones, without waiting.
///
/// A batch with a game material is drawn by its shader's pipeline (createMaterialPipeline(), on the game shaders'
/// layout), with its set 2. If the shader has instance values, they are a second vertex binding, locations 7 to 10,
/// and its draw starts at instance 0 with both buffers bound at the batch's first entries, since the two arrays are
/// not indexed alike.
/// @warning Destroy it before the Allocator and the Device.
class RTYPE_RENDER_VULKAN_API SpritePass {
  public:
    /// @brief Finds the descriptor set of a texture, or a null handle if it no longer exists.
    using TextureSetFinder = std::function<vk::DescriptorSet(plugins::backend::graphics::TextureId)>;

    /// @brief What a batch with a game material is drawn with.
    struct MaterialBinding {
        const pipeline::GraphicsPipeline* pipeline = nullptr;  ///< Its shader's; null: drawn as the default material.
        vk::DescriptorSet set;                                 ///< Its set 2, for this frame.
        bool instanceValues = false;                           ///< Its pipeline reads the instance values binding.
    };
    /// @brief Finds what a game material is drawn with this frame, or a null pipeline if it cannot be drawn.
    using MaterialFinder = std::function<MaterialBinding(plugins::backend::graphics::MaterialId)>;

    /// @param shaders Gives the built-in sprite and glyph shaders.
    /// @param layout The shared layout (ShaderLayout.hpp); must outlive the pass.
    /// @param colorFormat The format of the images drawn into: the swapchain's.
    /// @param cache Speeds up the pipeline's creation, or null.
    /// @throws vk::SystemError If a pipeline cannot be created.
    SpritePass(const core::Device& device, const memory::Allocator& allocator, pipeline::ShaderCache& shaders,
               const pipeline::PipelineLayout& layout, vk::Format colorFormat, const vk::raii::PipelineCache* cache,
               std::size_t framesInFlight);
    ~SpritePass() = default;
    SpritePass(const SpritePass&) = delete;
    SpritePass& operator=(const SpritePass&) = delete;
    SpritePass(SpritePass&&) = delete;
    SpritePass& operator=(SpritePass&&) = delete;

    /// @return The pipeline of a game shader's sprite variant: @p vertex and @p fragment, the SpriteInstance binding
    /// and, if @p instanceValues, the instance values binding, premultiplied alpha, on @p layout (the game shaders'
    /// layout).
    /// @throws vk::SystemError If the pipeline cannot be created.
    [[nodiscard]] static pipeline::GraphicsPipeline createMaterialPipeline(
        const core::Device& device, const vk::raii::ShaderModule& vertex, const vk::raii::ShaderModule& fragment,
        bool instanceValues, const pipeline::PipelineLayout& layout, vk::Format colorFormat,
        const vk::raii::PipelineCache* cache);

    /// @brief Copies @p batch's instances and instance values into frame @p frameIndex's buffers, for record().
    /// @throws vk::SystemError If a bigger buffer cannot be created.
    void upload(std::size_t frameIndex, const SpriteBatch& batch);

    /// @brief Records the draws of @p batches, some of the uploaded batch's, inside dynamic rendering, into
    /// @p viewport: binds the pipeline (so call it again after another pass recorded), then draws each batch, a batch
    /// with a game material through @p findMaterial (the default material if it gives no pipeline), a batch of glyphs
    /// with the glyph pipeline. The frame's set 0
    /// must already be bound. A batch whose texture no longer exists is skipped. Does nothing before upload().
    void record(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex,
                std::span<const SpriteBatch::Batch> batches, const plugins::backend::graphics::Viewport& viewport,
                const TextureSetFinder& findTextureSet, const MaterialFinder& findMaterial);

  private:
    /// @brief Instances the first buffer holds; each new one doubles until the frame's instances fit.
    static constexpr std::size_t kInitialCapacity = 1024;

    /// @return @p buffer, replaced by a bigger one if it holds fewer than @p count elements of @p elementSize bytes.
    memory::Buffer& reserve(std::optional<memory::Buffer>& buffer, std::size_t count, std::size_t elementSize);

    const memory::Allocator* _allocator;                  ///< Allocates the instance buffers; not owned.
    const pipeline::PipelineLayout* _layout;              ///< The shared layout; not owned.
    pipeline::GraphicsPipeline _pipeline;                 ///< Sprite shaders, instanced input, premultiplied alpha.
    pipeline::GraphicsPipeline _glyphPipeline;            ///< The same with the glyph fragment shader.
    std::vector<std::optional<memory::Buffer>> _buffers;  ///< One instance buffer per frame in flight.
    std::vector<std::optional<memory::Buffer>> _values;   ///< One instance values buffer per frame in flight.
};

}  // namespace rtype::render::vulkan::rendering
