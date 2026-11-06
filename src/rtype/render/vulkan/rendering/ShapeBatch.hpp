/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShapeBatch
*/

#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "ShapeInstance.hpp"
#include "render/vulkan/Export.hpp"
#include "rtype/contract/renderer/shaders/CompiledShader.hpp"

namespace rtype::render::vulkan::rendering {

/// @brief The shapes of a frame, in draw order, grouped in batches: consecutive shapes of the same material are drawn
/// by one instanced draw call.
///
/// @details Only consecutive shapes are merged, so the draw order (the last drawn on top) is kept. The shapes of a game
/// material also have their instance values, kept apart (getInstanceValues()), so the default ones pay nothing for
/// them.
class RTYPE_RENDER_VULKAN_API ShapeBatch {
  public:
    /// @brief Consecutive instances sharing a material.
    struct Batch {
        contract::renderer::MaterialId material;  ///< Its game material; the default one if 0.
        std::uint32_t firstInstance = 0;          ///< Index of its first instance in getInstances().
        std::uint32_t instanceCount = 0;          ///< Number of instances.
        std::uint32_t firstValues =
            0;  ///< Index of its first instance's values in getInstanceValues(), with a material.
    };

    ShapeBatch() = default;
    ~ShapeBatch() = default;
    ShapeBatch(const ShapeBatch&) = default;
    ShapeBatch& operator=(const ShapeBatch&) = default;
    ShapeBatch(ShapeBatch&&) noexcept = default;
    ShapeBatch& operator=(ShapeBatch&&) noexcept = default;

    /// @return The instances added since clear(), in draw order.
    [[nodiscard]] std::span<const ShapeInstance> getInstances() const noexcept { return _instances; }
    /// @return The instance values of the shapes drawn with a game material, in draw order.
    [[nodiscard]] std::span<const contract::renderer::ShaderInstanceValues> getInstanceValues() const noexcept {
        return _values;
    }
    /// @return The batches, in draw order.
    [[nodiscard]] std::span<const Batch> getBatches() const noexcept { return _batches; }

    /// @brief Adds @p instance, drawn with the default material, after the others: it extends the last batch if that
    /// one has the default material too, or starts a new one.
    void add(const ShapeInstance& instance);
    /// @brief Adds @p instance, drawn with the game material @p material with its @p values, as add().
    void add(contract::renderer::MaterialId material, const ShapeInstance& instance,
             const contract::renderer::ShaderInstanceValues& values);
    /// @brief Makes the next add() start a new batch: something else is drawn between the two.
    void split() noexcept;

    /// @brief Forgets every instance and batch, keeping the memory for the next frame.
    void clear() noexcept;

  private:
    /// @brief Starts a new batch unless the last one has @p material.
    void extend(contract::renderer::MaterialId material);

    std::vector<ShapeInstance> _instances;                          ///< In draw order.
    std::vector<contract::renderer::ShaderInstanceValues> _values;  ///< Of the material shapes, in order.
    std::vector<Batch> _batches;                                    ///< In draw order.
    bool _split = false;                                            ///< The next add() starts a new batch.
};

}  // namespace rtype::render::vulkan::rendering
