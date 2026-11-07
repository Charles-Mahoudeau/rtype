/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MeshPass
*/

#pragma once

#include <cstddef>
#include <functional>
#include <glm/ext/vector_uint2.hpp>
#include <optional>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"
#include "rtype/contract/renderer/drawing/Mesh.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"

namespace rtype::render::vulkan::rendering {

/// @brief Draws a contract::renderer::Mesh: indexed triangles in framebuffer pixels, premultiplied alpha, one scissored
/// indexed draw per command.
///
/// @details Owns the pipeline of shaders/mesh.vert and mesh.frag (on the shared layout) and, per frame in flight, a
/// vertex buffer and an index buffer, CPU-written. A frame's buffers are only used by that frame, whose fence was
/// waited for before recording, so they are rewritten, or replaced by bigger ones, without waiting. The frame's set 0
/// must hold a projection that maps framebuffer pixels (origin at the top-left corner) to clip space.
/// @warning Destroy it before the Allocator and the Device.
class RTYPE_RENDER_VULKAN_API MeshPass {
  public:
    /// @brief Finds the descriptor set (set 1) of a texture; a null handle when it no longer exists.
    using SetFinder = std::function<vk::DescriptorSet(contract::renderer::TextureId)>;

    /// @param shaders Gives the built-in mesh shaders.
    /// @param layout The shared layout (ShaderLayout.hpp); must outlive the pass.
    /// @param colorFormat The format of the images drawn into: the swapchain's.
    /// @param cache Speeds up the pipeline's creation, or null.
    /// @throws vk::SystemError If the pipeline cannot be created.
    MeshPass(const core::Device& device, const memory::Allocator& allocator, pipeline::ShaderCache& shaders,
             const pipeline::PipelineLayout& layout, vk::Format colorFormat, const vk::raii::PipelineCache* cache,
             std::size_t framesInFlight);
    ~MeshPass() = default;
    MeshPass(const MeshPass&) = delete;
    MeshPass& operator=(const MeshPass&) = delete;
    MeshPass(MeshPass&&) = delete;
    MeshPass& operator=(MeshPass&&) = delete;

    /// @brief Copies the vertices and indices of @p mesh into frame @p frameIndex's buffers, for record().
    /// @throws vk::SystemError If a bigger buffer cannot be created.
    void upload(std::size_t frameIndex, const contract::renderer::Mesh& mesh);

    /// @brief Records the commands of @p mesh, uploaded for this frame, inside dynamic rendering. The frame's set 0
    /// must already be bound. Nothing is recorded for an empty mesh or before upload().
    /// @param framebufferSize The size of the image drawn into: the viewport, and what the clips are clamped to.
    /// @param findSet Gives the set of a command's texture.
    /// @param whiteSet The set of a command with no texture, or whose texture no longer exists: a white pixel.
    void record(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex,
                const contract::renderer::Mesh& mesh, glm::uvec2 framebufferSize, const SetFinder& findSet,
                vk::DescriptorSet whiteSet);

  private:
    /// @return @p buffer, replaced by a bigger one if it holds less than @p count elements of @p elementSize bytes.
    memory::Buffer& reserve(std::optional<memory::Buffer>& buffer, std::size_t count, std::size_t elementSize,
                            vk::BufferUsageFlagBits usage, std::string_view name);

    const memory::Allocator* _allocator;                   ///< Allocates the buffers.
    pipeline::GraphicsPipeline _pipeline;                  ///< The mesh pipeline.
    std::vector<std::optional<memory::Buffer>> _vertices;  ///< The vertices, per frame in flight.
    std::vector<std::optional<memory::Buffer>> _indices;   ///< The indices, per frame in flight.
};

}  // namespace rtype::render::vulkan::rendering
