// The blend modes of sprites and shapes (plugins::backend::graphics::BlendMode, same values), drawn with one
// premultiplied-alpha pipeline (src + dst * (1 - src.a), GraphicsPipeline::BlendMode::kPremultipliedAlpha).
#ifndef RTYPE_BLEND_GLSL
#define RTYPE_BLEND_GLSL

const uint kBlendAlpha = 0u;     // Covers what is under it by its alpha.
const uint kBlendAdditive = 1u;  // Adds its colour, weighted by its alpha.
const uint kBlendCutout = 2u;    // Opaque or not drawn.

// A sprite's blend word also says how its texture is read (rendering/Blend.hpp, same values): bits 0 to 7 hold the
// blend mode, bit 8 kShaderMsdf, and bits 16 to 31 the distance range of an MSDF font atlas, in 1/256 of a pixel.
const uint kBlendMask = 0xFFu;
const uint kShaderMsdf = 0x100u;
const uint kDistanceRangeShift = 16u;
const float kDistanceRangeUnit = 256.0;

// Opacity under which a kBlendCutout fragment is not drawn.
const float kCutoutThreshold = 0.5;

// Returns what the premultiplied-alpha blend needs to draw `color` (premultiplied: rgb already times a) with `blend`.
vec4 finishBlend(vec4 color, uint blend) {
    if (blend == kBlendAdditive) {
        return vec4(color.rgb, 0.0);
    }
    if (blend == kBlendCutout) {
        // No discard: there is no depth buffer to keep the fragment out of, and a fully transparent premultiplied
        // colour blends to nothing (discard would also need the shaderDemoteToHelperInvocation feature under
        // Vulkan 1.3's SPIR-V).
        return color.a < kCutoutThreshold ? vec4(0.0) : vec4(color.rgb / color.a, 1.0);
    }
    return color;
}

#endif
