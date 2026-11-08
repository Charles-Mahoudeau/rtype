/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MsdfText
*/

#pragma once

#include <raylib.h>

#include "rtype/contract/renderer/drawing/Text.hpp"

namespace rtype::backend::raylib::renderer::draw {

/// @brief Draws the glyphs of a Text from its multi-channel signed distance field atlas, with a small fragment shader
/// (GLSL 330) that keeps the edges sharp at any size: the median of the three distances, smoothed over one screen
/// pixel.
class MsdfText {
  public:
    MsdfText() = default;
    /// @brief Frees the shader, if the window is still open.
    ~MsdfText();
    MsdfText(const MsdfText&) = delete;
    MsdfText& operator=(const MsdfText&) = delete;
    MsdfText(MsdfText&&) = delete;
    MsdfText& operator=(MsdfText&&) = delete;

    /// @brief Compiles the shader; needs the window's OpenGL context. Without it (a compile failure), glyphs are drawn
    /// as plain textures.
    void load();

    /// @brief Draws @p text with its font @p atlas.
    void draw(const ::Texture2D& atlas, const contract::renderer::Text& text) const;

  private:
    ::Shader _shader{};        ///< The MSDF shader.
    int _pxRangeLocation{-1};  ///< Its `pxRange` uniform: the atlas' distance range.
    bool _loaded{false};       ///< True once the shader compiled.
};

}  // namespace rtype::backend::raylib::renderer::draw
