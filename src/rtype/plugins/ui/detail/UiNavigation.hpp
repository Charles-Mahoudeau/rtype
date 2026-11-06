/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UiNavigation
*/

#pragma once

#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <optional>
#include <span>

#include "plugins/ui/Export.hpp"

namespace rtype::plugins::ui::detail {

/// @return The rectangle of @p centers nearest to @p from in the direction @p direction, a unit vector: what a press on
/// a directional key moves the focus to. Nothing if none is that way.
[[nodiscard]] RTYPE_PLUGIN_UI_API std::optional<std::size_t> findNext(std::span<const glm::vec2> centers,
                                                                      glm::vec2 from, glm::vec2 direction);

}  // namespace rtype::plugins::ui::detail
