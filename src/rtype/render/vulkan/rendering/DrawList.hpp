/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DrawList
*/

#pragma once

#include <cstdint>
#include <variant>
#include <vector>

#include "render/vulkan/Export.hpp"

namespace rtype::render::vulkan::rendering {

/// @brief The order of a frame's draws: runs of sprite batches (SpriteBatch), with the particle draws recorded between
/// them, so a particle drawn between two sprites is drawn between them on screen.
///
/// @details The renderer records each particle draw with the number of sprite batches before it, and splits the
/// SpriteBatch there (SpriteBatch::split()), so no batch spans a particle draw. getSteps() then gives the runs and the
/// particle draws in order.
class RTYPE_RENDER_VULKAN_API DrawList {
  public:
    /// @brief Sprite batches drawn one after the other: a range of SpriteBatch::getBatches().
    struct SpriteRun {
        std::uint32_t firstBatch = 0;  ///< Index of the first batch.
        std::uint32_t batchCount = 0;  ///< Number of batches, at least 1.

        [[nodiscard]] bool operator==(const SpriteRun& other) const = default;
    };

    /// @brief One particle draw, by its index in the order the renderer recorded them.
    struct ParticleStep {
        std::uint32_t draw = 0;  ///< Index of the particle draw.

        [[nodiscard]] bool operator==(const ParticleStep& other) const = default;
    };

    /// @brief One thing to draw: a run of sprite batches or a particle draw.
    using Step = std::variant<SpriteRun, ParticleStep>;

    DrawList() = default;
    ~DrawList() = default;
    DrawList(const DrawList&) = default;
    DrawList& operator=(const DrawList&) = default;
    DrawList(DrawList&&) noexcept = default;
    DrawList& operator=(DrawList&&) noexcept = default;

    /// @brief Records particle draw @p draw after the first @p batchCount sprite batches of the frame.
    void addParticles(std::uint32_t batchCount, std::uint32_t draw);

    /// @return The steps of a frame of @p batchCount sprite batches in all, in draw order: no empty run.
    [[nodiscard]] std::vector<Step> getSteps(std::uint32_t batchCount) const;

    /// @brief Forgets every particle draw, keeping the memory for the next frame.
    void clear() noexcept;

  private:
    /// @brief A particle draw and the number of sprite batches before it.
    struct Marker {
        std::uint32_t batchCount = 0;  ///< Sprite batches drawn before it.
        std::uint32_t draw = 0;        ///< Index of the particle draw.
    };

    std::vector<Marker> _markers;  ///< In draw order.
};

}  // namespace rtype::render::vulkan::rendering
