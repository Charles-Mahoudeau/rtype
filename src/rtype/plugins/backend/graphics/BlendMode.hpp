/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** BlendMode
*/

#pragma once

#include <cstdint>

namespace rtype::plugins::backend::graphics {

/// @brief How a sprite or a shape is combined with what is already drawn under it.
/// @details A backend may draw every mode with one pipeline (Vulkan does, with premultiplied alpha), so changing the
/// mode of a sprite never splits its batch: only its texture does.
enum class BlendMode : std::uint8_t {
    kAlpha,     ///< Covers what is under it by its alpha: the usual transparency.
    kAdditive,  ///< Adds its colour, weighted by its alpha, without darkening: glows, lasers, explosions.
    kCutout,  ///< Each pixel is either drawn opaque or not at all (under half opacity): crisp pixel art edges. Sprites
              ///< only: a shape drawn with it is drawn as kAlpha.
};

}  // namespace rtype::plugins::backend::graphics
