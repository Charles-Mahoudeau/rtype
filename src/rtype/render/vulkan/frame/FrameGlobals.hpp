/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FrameGlobals
*/

#pragma once

#include <cstddef>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/descriptor/DescriptorPool.hpp"
#include "render/vulkan/descriptor/DescriptorSetLayout.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/rendering/FrameUniforms.hpp"

namespace rtype::render::vulkan::frame {

/// @brief The data every shader can read during a frame (rendering::FrameUniforms), at set 0, binding 0.
///
/// @details Holds one uniform buffer and one descriptor set per frame in flight: while the GPU still reads frame N's
/// buffer, the CPU writes frame N + 1's, so update() never has to wait for the GPU. Each set points at its own buffer
/// once and for all (written by the constructor), so a frame only writes its buffer and binds its set:
/// @code
/// frameFence.wait();                        // the GPU is done with this frame's buffer
/// globals.update(frameIndex, uniforms);     // so the CPU can overwrite it
/// globals.bind(commandBuffer, layout, frameIndex);
/// @endcode
/// Every pipeline layout that reads the uniforms takes getSetLayout() as its set 0.
/// @warning Destroy it before the Allocator and the Device.
class RTYPE_RENDER_VULKAN_API FrameGlobals {
  public:
    /// @param device Creates the layout, the pool and the sets; must outlive this object.
    /// @param allocator Allocates the uniform buffers; must outlive this object.
    /// @param framesInFlight Number of buffers and sets: one per frame the CPU records while the GPU renders another.
    /// @throws vk::SystemError If a Vulkan object cannot be created.
    FrameGlobals(const core::Device& device, const memory::Allocator& allocator, std::size_t framesInFlight);
    ~FrameGlobals() = default;
    FrameGlobals(const FrameGlobals&) = delete;
    FrameGlobals& operator=(const FrameGlobals&) = delete;
    FrameGlobals(FrameGlobals&&) = delete;
    FrameGlobals& operator=(FrameGlobals&&) = delete;

    /// @return The layout of the sets, to put at index 0 of the pipeline layouts that read the uniforms.
    [[nodiscard]] const descriptor::DescriptorSetLayout& getSetLayout() const noexcept { return _setLayout; }

    /// @brief Writes @p uniforms into the buffer of frame @p frameIndex.
    /// @warning Only once that frame's fence was waited for: the GPU may still be reading the buffer before.
    /// @throws std::out_of_range If @p frameIndex is not lower than the number of frames in flight.
    void update(std::size_t frameIndex, const rendering::FrameUniforms& uniforms);

    /// @brief Binds the set of frame @p frameIndex at set 0 for the graphics pipelines.
    /// @param layout A pipeline layout whose set 0 is getSetLayout().
    /// @throws std::out_of_range If @p frameIndex is not lower than the number of frames in flight.
    void bind(const vk::raii::CommandBuffer& commandBuffer, const pipeline::PipelineLayout& layout,
              std::size_t frameIndex) const;

  private:
    // Destroyed in reverse order: the sets first (they give their descriptors back to the pool), the pool last.
    descriptor::DescriptorSetLayout _setLayout;  ///< One uniform buffer at binding 0, for every shader stage.
    descriptor::DescriptorPool _pool;            ///< Allocates the sets; declared before them, so destroyed after.
    std::vector<memory::Buffer> _buffers;        ///< One rendering::FrameUniforms per frame in flight, CPU-writable.
    std::vector<vk::raii::DescriptorSet> _sets;  ///< One per frame in flight, pointing at the buffer of same index.
};

}  // namespace rtype::render::vulkan::frame
