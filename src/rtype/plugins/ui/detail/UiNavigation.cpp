/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UiNavigation
*/

#include "plugins/ui/detail/UiNavigation.hpp"

#include <cmath>
#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <glm/geometric.hpp>
#include <optional>
#include <span>

namespace rtype::plugins::ui::detail {

std::optional<std::size_t> findNext(const std::span<const glm::vec2> centers, const glm::vec2 from,
                                    const glm::vec2 direction) {
    // Off the axis weighs more than along it: the next item of a column wins over a nearer one of another column.
    constexpr float kSidePenalty = 3.0F;
    std::optional<std::size_t> best;
    float bestScore = 0.0F;
    for (std::size_t index = 0; index < centers.size(); ++index) {
        const glm::vec2 offset = centers[index] - from;
        const float along = glm::dot(offset, direction);
        if (along <= 1.0F) {
            continue;
        }
        const float side = std::abs((offset.x * direction.y) - (offset.y * direction.x));
        const float score = along + (side * kSidePenalty);
        if (!best || score < bestScore) {
            best = index;
            bestScore = score;
        }
    }
    return best;
}
}  // namespace rtype::plugins::ui::detail
