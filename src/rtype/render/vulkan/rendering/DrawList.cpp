/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DrawList
*/

#include "DrawList.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace rtype::render::vulkan::rendering {

void DrawList::addParticles(std::uint32_t batchCount, std::uint32_t draw) {
    _markers.push_back(Marker{.batchCount = batchCount, .draw = draw});
}

std::vector<DrawList::Step> DrawList::getSteps(std::uint32_t batchCount) const {
    std::vector<Step> steps;
    std::uint32_t next = 0;
    for (const Marker& marker : _markers) {
        const std::uint32_t end = std::min(marker.batchCount, batchCount);
        if (end > next) {
            steps.emplace_back(SpriteRun{.firstBatch = next, .batchCount = end - next});
            next = end;
        }
        steps.emplace_back(ParticleStep{.draw = marker.draw});
    }
    if (batchCount > next) {
        steps.emplace_back(SpriteRun{.firstBatch = next, .batchCount = batchCount - next});
    }
    return steps;
}

void DrawList::clear() noexcept { _markers.clear(); }

}  // namespace rtype::render::vulkan::rendering
