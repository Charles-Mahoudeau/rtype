// The texture being drawn and its sampler, at set 1, binding 0 (rendering/ShaderLayout.hpp: kTextureSet,
// kTextureBinding). Read it in a fragment shader with texture(uTexture, uv).
#ifndef RTYPE_TEXTURE_SET_GLSL
#define RTYPE_TEXTURE_SET_GLSL

layout(set = 1, binding = 0) uniform sampler2D uTexture;

#endif
