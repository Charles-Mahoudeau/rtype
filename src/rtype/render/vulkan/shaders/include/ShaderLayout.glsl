// Everything a shader of the renderer can read, grouped by how often it changes (the GPU side of
// rendering/ShaderLayout.hpp):
//   set 0, binding 0   frame      FrameUniforms.glsl   once per frame
//   set 1, binding 0   uTexture   TextureSet.glsl      per texture (batch)
//   push constants     draw       DrawConstants.glsl   per draw
// Every pipeline the renderer builds uses this layout, so a shader may read any of them (uTexture from fragment
// shaders only). Use it with:
//     #extension GL_GOOGLE_include_directive : require
//     #include "ShaderLayout.glsl"
#ifndef RTYPE_SHADER_LAYOUT_GLSL
#define RTYPE_SHADER_LAYOUT_GLSL

#include "DrawConstants.glsl"
#include "FrameUniforms.glsl"
#include "TextureSet.glsl"

#endif
