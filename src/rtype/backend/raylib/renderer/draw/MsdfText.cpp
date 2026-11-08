/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MsdfText
*/

#include "MsdfText.hpp"

#include <raylib.h>

#include <glm/ext/vector_float2.hpp>

#include "backend/raylib/renderer/Convert.hpp"
#include "rtype/contract/renderer/drawing/Text.hpp"

namespace rtype::backend::raylib::renderer::draw {
namespace {

/// @brief The MSDF fragment shader; raylib's default vertex shader feeds it.
constexpr const char* kFragmentShader = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float pxRange;
out vec4 finalColor;
float median(float r, float g, float b) { return max(min(r, g), min(max(r, g), b)); }
void main() {
    vec3 msd = texture(texture0, fragTexCoord).rgb;
    vec2 unitRange = vec2(pxRange) / vec2(textureSize(texture0, 0));
    vec2 screenTexSize = vec2(1.0) / fwidth(fragTexCoord);
    float screenPxRange = max(0.5 * dot(unitRange, screenTexSize), 1.0);
    float opacity = clamp(screenPxRange * (median(msd.r, msd.g, msd.b) - 0.5) + 0.5, 0.0, 1.0);
    vec4 color = fragColor * colDiffuse;
    finalColor = vec4(color.rgb, color.a * opacity);
}
)";

}  // namespace

MsdfText::~MsdfText() {
    if (_loaded && IsWindowReady()) {
        UnloadShader(_shader);
    }
}

void MsdfText::load() {
    _shader = LoadShaderFromMemory(nullptr, kFragmentShader);
    _loaded = IsShaderValid(_shader);
    if (_loaded) {
        _pxRangeLocation = GetShaderLocation(_shader, "pxRange");
    }
}

void MsdfText::draw(const ::Texture2D& atlas, const contract::renderer::Text& text) const {
    if (_loaded) {
        BeginShaderMode(_shader);
        SetShaderValue(_shader, _pxRangeLocation, &text.distanceRange, SHADER_UNIFORM_FLOAT);
    }
    const ::Color color = toColor(text.color);
    const float degrees = text.rotation * kDegreesPerRadian;
    for (const contract::renderer::GlyphQuad& glyph : text.glyphs) {
        // Every glyph turns around the text's origin: the destination is the text's position, and the glyph's corner
        // is placed relative to that origin.
        const glm::vec2 corner{glyph.bounds.getX(), glyph.bounds.getY()};
        const ::Rectangle destination{.x = text.position.x,
                                      .y = text.position.y,
                                      .width = glyph.bounds.getWidth() * text.scale.x,
                                      .height = glyph.bounds.getHeight() * text.scale.y};
        DrawTexturePro(atlas, toRectangle(glyph.source), destination, toVector((text.origin - corner) * text.scale),
                       degrees, color);
    }
    if (_loaded) {
        EndShaderMode();
    }
}

}  // namespace rtype::backend::raylib::renderer::draw
